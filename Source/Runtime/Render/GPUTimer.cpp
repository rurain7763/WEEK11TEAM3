#include "EnginePCH.h"
#include "Render/GPUTimer.h"

#include "Render/RenderCommand.h"

namespace
{
	Microsoft::WRL::ComPtr<ID3D11Query> CreateQuery(D3D11_QUERY Type)
	{
		D3D11_QUERY_DESC Desc{};
		Desc.Query = Type;
		Microsoft::WRL::ComPtr<ID3D11Query> Query;
		RenderCommand::GetDevice()->CreateQuery(&Desc, Query.GetAddressOf());
		return Query;
	}
}

void FGPUTimer::BeginFrame()
{
	ID3D11DeviceContext* Context = RenderCommand::GetContext();

	// 이전 프레임 마무리
	if (Current >= 0)
	{
		if (bScopeOpen)
		{
			EndScope();
		}
		FFrameQueries& Prev = Frames[Current];
		Context->End(Prev.Disjoint.Get());
		Prev.bIssued = true;
	}

	Current = (Current + 1) % FrameLatency;
	FFrameQueries& Frame = Frames[Current];

	// 이 슬롯을 다시 쓰기 전에 4프레임 전 결과를 읽는다
	if (Frame.bIssued)
	{
		Resolve(Frame);
		Frame.bIssued = false;
	}

	if (!Frame.Disjoint)
	{
		Frame.Disjoint = CreateQuery(D3D11_QUERY_TIMESTAMP_DISJOINT);
	}
	Context->Begin(Frame.Disjoint.Get());
	Frame.Used = 0;
}

void FGPUTimer::BeginScope()
{
	if (Current < 0 || bScopeOpen)
	{
		return;
	}

	FFrameQueries& Frame = Frames[Current];
	while (Frame.Stamps.Num() < Frame.Used + 2)
	{
		Frame.Stamps.Add(CreateQuery(D3D11_QUERY_TIMESTAMP));
	}

	// 타임스탬프 쿼리는 Begin 없이 End만 호출한다
	RenderCommand::GetContext()->End(Frame.Stamps[Frame.Used].Get());
	bScopeOpen = true;
}

void FGPUTimer::EndScope()
{
	if (Current < 0 || !bScopeOpen)
	{
		return;
	}

	FFrameQueries& Frame = Frames[Current];
	RenderCommand::GetContext()->End(Frame.Stamps[Frame.Used + 1].Get());
	Frame.Used += 2;
	bScopeOpen = false;
}

void FGPUTimer::Resolve(FFrameQueries& Frame)
{
	ID3D11DeviceContext* Context = RenderCommand::GetContext();
	constexpr UINT Flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;

	// 아직 안 끝났으면 이번 샘플은 버린다 (CPU를 멈추지 않는다)
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData{};
	if (Context->GetData(Frame.Disjoint.Get(), &DisjointData, sizeof(DisjointData), Flags) != S_OK)
	{
		return;
	}
	if (DisjointData.Disjoint || DisjointData.Frequency == 0)
	{
		return;
	}

	uint64 TotalTicks = 0;
	for (int32 i = 0; i + 1 < Frame.Used; i += 2)
	{
		uint64 Begin = 0;
		uint64 End = 0;
		if (Context->GetData(Frame.Stamps[i].Get(), &Begin, sizeof(Begin), Flags) != S_OK
			|| Context->GetData(Frame.Stamps[i + 1].Get(), &End, sizeof(End), Flags) != S_OK)
		{
			return;
		}
		TotalTicks += End - Begin;
	}

	const float Ms = static_cast<float>(static_cast<double>(TotalTicks) * 1000.0 / static_cast<double>(DisjointData.Frequency));
	// 매 프레임 값은 흔들리므로 평활한다
	SmoothedMs = SmoothedMs == 0.0f ? Ms : SmoothedMs * 0.9f + Ms * 0.1f;
}

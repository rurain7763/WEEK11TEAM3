#pragma once

#include "Core/Types.h"
#include "Containers/Array.h"

#include <d3d11.h>
#include <wrl/client.h>

// D3D11 타임스탬프 쿼리로 한 프레임 안의 여러 구간 GPU 시간을 합산한다.
// GPU는 몇 프레임 늦게 끝나므로 4프레임 전 결과를 읽어 CPU가 기다리지 않게 한다
class FGPUTimer
{
public:
	void BeginFrame();
	void BeginScope();
	void EndScope();

	// 지수 평활한 ms. 측정 전이면 0
	float GetSmoothedMs() const { return SmoothedMs; }

private:
	static constexpr int32 FrameLatency = 4;

	struct FFrameQueries
	{
		Microsoft::WRL::ComPtr<ID3D11Query> Disjoint;
		TArray<Microsoft::WRL::ComPtr<ID3D11Query>> Stamps;   // [Begin, End, Begin, End, ...]
		int32 Used = 0;
		bool bIssued = false;
	};

	void Resolve(FFrameQueries& Frame);

	FFrameQueries Frames[FrameLatency];
	int32 Current = -1;
	bool bScopeOpen = false;
	float SmoothedMs = 0.0f;
};

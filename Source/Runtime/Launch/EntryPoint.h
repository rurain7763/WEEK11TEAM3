#pragma once

#include "EnginePCH.h"
#include "LaunchEngineLoop.h"
#include "Core/EngineLog.h"
#include <cstdio>
#include <Windows.h>

#if defined(ENGINE_DEBUG)
namespace
{
	class FDebugConsole final : public ILogSink
	{
	public:
		FDebugConsole()
		{
			bOwnsConsole = AllocConsole() != FALSE;
			if (!bOwnsConsole) return;

			SetConsoleOutputCP(CP_UTF8);
			FILE* Stream = nullptr;
			freopen_s(&Stream, "CONOUT$", "w", stdout);
			freopen_s(&Stream, "CONOUT$", "w", stderr);
			FLog::AddSink(this);
		}

		~FDebugConsole()
		{
			if (!bOwnsConsole) return;
			FLog::RemoveSink(this);
			std::fflush(stdout);
			std::fflush(stderr);
			FreeConsole();
		}

		void OnLog(ELogVerbosity Verbosity, const FString& Message) override
		{
			FILE* Output = Verbosity == ELogVerbosity::Error ? stderr : stdout;
			std::fwrite(Message.data(), 1, Message.size(), Output);
			std::fputc('\n', Output);
			std::fflush(Output);
		}

	private:
		bool bOwnsConsole = false;
	};
}
#endif

// 실행 파일이 어떤 엔진 모드(UEngine 서브클래스)로 돌지 정한다. UE의 ini GameEngine= 설정 역할.
extern UClass* GetEngineClass();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#if defined(ENGINE_DEBUG)
	FDebugConsole DebugConsole;
#endif

	FEngineLoop EngineLoop;
	if (!EngineLoop.PreInit(hInstance, GetEngineClass())) return -1;

	while (EngineLoop.Tick())
	{
	}

	EngineLoop.Exit();

	if (FEngineStatics::TotalAllocationCount > 0)
		OutputDebugStringA("leaked UObjects\n");

	return 0;
}

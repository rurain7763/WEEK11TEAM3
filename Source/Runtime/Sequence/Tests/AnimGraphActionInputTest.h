#pragma once

#include "Animation/AnimInstance.h"
#include "Input/InputSystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// Temporary input harness only. Uses the existing graph-created instance;
// never creates/replaces an instance, builds nodes, or advances playback itself.
struct FAnimGraphActionInputTest
{
	static bool HasTriggers(UAnimInstance* Instance)
	{
		if (!Instance) return false;
		for (const char* Text : { "Attack", "Hit", "Jump" })
		{
			const FName Name(Text);
			if (!Instance->HasParameter(Name)) return false;
		}
		return true;
	}

	static void Announce(UAnimInstance* Instance, const FString& Owner)
	{
		if (HasTriggers(Instance))
			HTR_LOG(Info, "[AnimGraph input test] {}: focus the Play viewport; 1=Attack, 2=Hit, 3=Jump. Actions return automatically via ExitTime; 0 clears input.", Owner);
	}

	static void Update(UAnimInstance* Instance, UWorld* World, const FString& Owner)
	{
		const auto* Controller = World ? World->GetPlayerController() : nullptr;
		if (!Controller || !Controller->IsInputEnabled()) return;
		if (!HasTriggers(Instance)) return;
		const char* Triggers[] = { "Attack", "Hit", "Jump" };
		const EKeyCode Keys[] = { EKeyCode::Num0, EKeyCode::Num1, EKeyCode::Num2, EKeyCode::Num3 };
		const EKeyCode PadKeys[] = { EKeyCode::NumPad0, EKeyCode::NumPad1, EKeyCode::NumPad2, EKeyCode::NumPad3 };
		for (int32 Value = 0; Value < 4; ++Value)
		{
			if (!FInputSystem::IsKeyPressed(Keys[Value]) && !FInputSystem::IsKeyPressed(PadKeys[Value])) continue;
			if (Value == 0)
			{
				for (const char* Trigger : Triggers) Instance->ResetTrigger(FName(Trigger));
			}
			else Instance->SetTrigger(FName(Triggers[Value - 1]));
			HTR_LOG(Info, "[AnimGraph input test] {}: {}", Owner, Value == 0 ? "Cleared requests" : Triggers[Value - 1]);
			break;
		}
	}
};

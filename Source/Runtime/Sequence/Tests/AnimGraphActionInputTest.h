#pragma once

#include "Animation/AnimInstance.h"
#include "Input/InputSystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// Temporary input harness only. Uses the existing graph-created instance;
// never creates/replaces an instance, builds nodes, or advances playback itself.
struct FAnimGraphActionInputTest
{
	static FAnimParameter* FindAction(UAnimInstance* Instance)
	{
		const FName Name("Action");
		if (!Instance || !Instance->ParameterTable.HasParameter(Name)) return nullptr;
		const auto& Parameter = Instance->ParameterTable.FindParameter(Name);
		if (Parameter.Type != EAnimParameterType::Int32) return nullptr;
		// The table currently exposes const lookup only; this object itself is mutable.
		return const_cast<FAnimParameter*>(&Parameter);
	}

	static void Announce(UAnimInstance* Instance, const FString& Owner)
	{
		if (FindAction(Instance))
			HTR_LOG(Info, "[AnimGraph input test] {}: focus the Play viewport; 0=Locomotion, 1=Attack, 2=Hit, 3=Jump. Return with 0.", Owner);
	}

	static void Update(UAnimInstance* Instance, UWorld* World, const FString& Owner)
	{
		const auto* Controller = World ? World->GetPlayerController() : nullptr;
		if (!Controller || !Controller->IsInputEnabled()) return;
		FAnimParameter* Action = FindAction(Instance);
		if (!Action) return;
		const EKeyCode Keys[] = { EKeyCode::Num0, EKeyCode::Num1, EKeyCode::Num2, EKeyCode::Num3 };
		const EKeyCode PadKeys[] = { EKeyCode::NumPad0, EKeyCode::NumPad1, EKeyCode::NumPad2, EKeyCode::NumPad3 };
		for (int32 Value = 0; Value < 4; ++Value)
		{
			if (!FInputSystem::IsKeyPressed(Keys[Value]) && !FInputSystem::IsKeyPressed(PadKeys[Value])) continue;
			Action->Value.Int32 = Value;
			HTR_LOG(Info, "[AnimGraph input test] {}: Action={}", Owner, Value);
			break;
		}
	}
};

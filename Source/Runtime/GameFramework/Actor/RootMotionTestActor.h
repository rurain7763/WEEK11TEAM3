#pragma once

#include "GameFramework/Pawn.h"

class USkeletalMeshComponent;
class UCharacterMovementComponent;
class UScriptComponent;
class UCapsuleComponent;
class FAnimNotifyEvent;

class ARootMotionTestActor : public APawn
{
	DECLARE_CLASS(ARootMotionTestActor, APawn)

public:
	ARootMotionTestActor();

	virtual void BeginPlay() override;

	USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
	UCharacterMovementComponent* GetCharacterMovementComponent() const { return CharacterMovementComponent; }

private:
	void HandleNotify(const FAnimNotifyEvent& Event);

private:
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	UCharacterMovementComponent* CharacterMovementComponent = nullptr;
	UScriptComponent* ScriptComponent = nullptr;
	UCapsuleComponent* CapsuleComponent = nullptr;
};

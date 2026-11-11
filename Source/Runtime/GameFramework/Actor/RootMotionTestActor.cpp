#include "EnginePCH.h"
#include "RootMotionTestActor.h"

ARootMotionTestActor::ARootMotionTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
	SkeletalMeshComponent->SetupAttachment(GetRootComponent());

	CharacterMovementComponent = CreateDefaultSubobject<UCharacterMovementComponent>("CharacterMovementComponent");
	CharacterMovementComponent->Gravity = 0.0f;

	CharacterMovementComponent->SetMeshComponent(SkeletalMeshComponent);

	ScriptComponent = CreateDefaultSubobject<UScriptComponent>("ScriptComponent");
	ScriptComponent->SetScriptPath("Assets/Scripts/Actors/AnimTest.lua");
}

#include "EnginePCH.h"
#include "RootMotionTestActor.h"

ARootMotionTestActor::ARootMotionTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("USkeletalMeshComponent");
	SkeletalMeshComponent->SetupAttachment(GetRootComponent());

	CharacterMovementComponent = CreateDefaultSubobject<UCharacterMovementComponent>("UCharacterMovementComponent");
	CharacterMovementComponent->Gravity = 20.0f;
	CharacterMovementComponent->SetCollisionComponent(BoxComponent);

	SetMeshComponent(SkeletalMeshComponent);

	ScriptComponent = CreateDefaultSubobject<UScriptComponent>("UScriptComponent");
	ScriptComponent->SetScriptPath("Assets/Scripts/Actors/AnimTest.lua");
}

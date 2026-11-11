#include "EnginePCH.h"
#include "RootMotionTestActor.h"

#include "Component/SkeletalMeshComponent.h"
#include "Component/CharacterMovementComponent.h"
#include "Component/ScriptComponent.h"

ARootMotionTestActor::ARootMotionTestActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
	SkeletalMeshComponent->SetupAttachment(GetRootComponent());

	CharacterMovementComponent = CreateDefaultSubobject<UCharacterMovementComponent>("CharacterMovementComponent");
	// 루트 모션만으로 움직이는지 보려는 액터라 중력으로 떨어지지 않게 한다.
	CharacterMovementComponent->Gravity = 0.0f;
	// 이 연결이 있어야 CharacterMovementComponent가 매 프레임 루트 모션을 꺼내 이동에 반영한다.
	CharacterMovementComponent->SetMeshComponent(SkeletalMeshComponent);

	ScriptComponent = CreateDefaultSubobject<UScriptComponent>("ScriptComponent");
	ScriptComponent->SetScriptPath("Assets/Scripts/Actors/AnimTest.lua");
}

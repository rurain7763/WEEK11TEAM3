#include "EnginePCH.h"
#include "RootMotionTestActor.h"

#include "Component/SkeletalMeshComponent.h"
#include "Component/CharacterMovementComponent.h"
#include "Component/ScriptComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimNotify.h"

ARootMotionTestActor::ARootMotionTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// AnimTest.lua가 이동 입력이 있을 때만 캐릭터를 카메라 방향으로 돌린다.
	bUseControllerRotationYaw = false;

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

void ARootMotionTestActor::BeginPlay()
{
	Super::BeginPlay();

	auto* Instance = SkeletalMeshComponent->GetAnimInstance();
	if (Instance)
	{
		Instance->OnFireNotify.AddRaw(this, &ARootMotionTestActor::HandleNotify);
	}
}

void ARootMotionTestActor::HandleNotify(const FAnimNotifyEvent& Event)
{
	// TODO: Do something
	// HTR_LOG(Info, "Fire {}", Event.Name.ToString());
	HTR_LOG(Info, "Fire {} @ {:.3f}s", Event.Name.ToString(), Event.TriggerTime);
}

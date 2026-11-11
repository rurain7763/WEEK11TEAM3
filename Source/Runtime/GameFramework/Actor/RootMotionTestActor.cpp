#include "EnginePCH.h"
#include "RootMotionTestActor.h"

#include "Component/SkeletalMeshComponent.h"
#include "Component/CharacterMovementComponent.h"
#include "Component/ScriptComponent.h"
#include "Component/CapsuleComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimNotify.h"

ARootMotionTestActor::ARootMotionTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// AnimTest.lua가 이동 입력이 있을 때만 캐릭터를 카메라 방향으로 돌린다.
	bUseControllerRotationYaw = false;

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
	SkeletalMeshComponent->SetupAttachment(GetRootComponent());

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	CapsuleComponent->SetupAttachment(GetRootComponent());
	CapsuleComponent->SetBlockComponent(true);

	CharacterMovementComponent = CreateDefaultSubobject<UCharacterMovementComponent>("CharacterMovementComponent");
	CharacterMovementComponent->Gravity = 20.0f;
	CharacterMovementComponent->SetCollisionComponent(CapsuleComponent);
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

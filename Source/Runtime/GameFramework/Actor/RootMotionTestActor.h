#pragma once

#include "GameFramework/Pawn.h"

class USkeletalMeshComponent;
class UCharacterMovementComponent;
class UScriptComponent;

// 루트 모션 확인용 테스트 폰.
// Lua가 애니메이션 파라미터만 세팅하고, 실제 이동은 재생 중인 클립의 루트 모션이 만든다.
// (UCharacterMovementComponent가 MeshComponent에서 루트 모션을 꺼내 적용한다)
class ARootMotionTestActor : public APawn
{
	DECLARE_CLASS(ARootMotionTestActor, APawn)

public:
	ARootMotionTestActor();

	USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
	UCharacterMovementComponent* GetCharacterMovementComponent() const { return CharacterMovementComponent; }

private:
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	UCharacterMovementComponent* CharacterMovementComponent = nullptr;
	UScriptComponent* ScriptComponent = nullptr;
};

#pragma once

#include "GameFramework/Pawn.h"
#include "Component/CharacterMovementComponent.h"
#include "Component/ScriptComponent.h"
#include "Component/SkeletalMeshComponent.h"


class ARootMotionTestActor : public APawn
{
    DECLARE_CLASS(ARootMotionTestActor, APawn)

public:
    ARootMotionTestActor();

private:
    USkeletalMeshComponent * SkeletalMeshComponent = nullptr;
    UCharacterMovementComponent* CharacterMovementComponent = nullptr;
    UScriptComponent* ScriptComponent = nullptr;
};
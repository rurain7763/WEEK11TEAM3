#pragma once

#include "Containers/Array.h"
#include "Math/Transform.h"

class FAnimParameterTable;
struct FAnimNotifyEventReference;

struct FPoseContext
{
	int32 BoneCount;
	TArray<FVector> BoneTranslations;
	TArray<FQuat> BoneRotations;
	TArray<FVector> BoneScales;
};

struct FAnimExtractContext
{
	float CurrentTime = 0.f;
	bool bLooping = false;
	bool bExtractRootMotion = false;
};

struct FAnimationUpdateContext
{
	FAnimParameterTable* ParameterTable;
	TArray<FAnimNotifyEventReference>* EventQueue;
	bool bHasRootMotion;
	FTransform RootMotion;
	float DeltaTime = 0.f;
};

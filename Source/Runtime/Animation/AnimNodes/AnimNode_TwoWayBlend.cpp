#include "EnginePCH.h"
#include "Animation/AnimNodes/AnimNode_TwoWayBlend.h"
#include "Animation/AnimationRuntime.h"

void FAnimNode_TwoWayBlend::Update(FAnimationUpdateContext& UpdateContext)
{
	FAnimationUpdateContext ContextA = UpdateContext;
	ContextA.bHasRootMotion = false;
	ContextA.RootMotion = FTransform::Identity;
	A->Update(ContextA);

	FAnimationUpdateContext ContextB = UpdateContext;
	ContextB.bHasRootMotion = false;
	ContextB.RootMotion = FTransform::Identity;
	B->Update(ContextB);

	if (Alpha == 0.f)
	{
		UpdateContext.bHasRootMotion |= ContextA.bHasRootMotion;
	}
	else if (Alpha == 1.f)
	{
		UpdateContext.bHasRootMotion |= ContextB.bHasRootMotion;
	}
	else
	{
		UpdateContext.bHasRootMotion |= ContextA.bHasRootMotion | ContextB.bHasRootMotion;
	}

	UpdateContext.RootMotion = FTransform::Blend(ContextA.RootMotion, ContextB.RootMotion, Alpha);
}

void FAnimNode_TwoWayBlend::Evaluate(FPoseContext& OutPoseData)
{
	FPoseContext PoseA;
	A->Evaluate(PoseA);

	FPoseContext PoseB;
	B->Evaluate(PoseB);

	FAnimationRuntime::BlendTwoPosesTogether(PoseA, PoseB, Alpha, OutPoseData);
}

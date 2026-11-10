#pragma once

#include "Animation/AnimationTypes.h"

struct FAnimationRuntime
{
	static void BlendTwoPosesTogether(const FPoseContext& PoseA, const FPoseContext& PoseB, float T, FPoseContext& OutPose);
};

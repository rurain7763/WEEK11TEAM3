#pragma once

#include "Animation/AnimationTypes.h"

struct FAnimNode_Base
{
	virtual void Initialize() {}
	virtual void Update(FAnimationUpdateContext& UpdateContext) {}
	virtual void Evaluate(FPoseContext& OutPoseContext) {}
};

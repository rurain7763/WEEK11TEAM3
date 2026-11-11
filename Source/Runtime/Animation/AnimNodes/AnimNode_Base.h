#pragma once

#include "Animation/AnimationTypes.h"

struct FAnimNode_Base
{
	virtual ~FAnimNode_Base() = default;

	virtual void Initialize() {}
	virtual void Update(FAnimationUpdateContext& UpdateContext) {}
	virtual void Evaluate(FPoseContext& OutPoseContext) {}

	// NOTE: -1은 시간 조회가 불가능한 Node라는 의미.
	virtual float GetNormalizedTime() const { return -1.f; }
	virtual float GetSpeed() const { return 1.f; }
};

#pragma once

#include "Animation/AnimNodes/AnimNode_Base.h"

struct FAnimNode_TwoWayBlend : public FAnimNode_Base
{
	FAnimNode_Base* A;
	FAnimNode_Base* B;
	float Alpha = 0.f;

	void Initialize() override
	{
		A->Initialize();
		B->Initialize();
	}

	void Update(FAnimationUpdateContext& UpdateContext) override;

	void Evaluate(FPoseContext& OutPoseData) override;
};

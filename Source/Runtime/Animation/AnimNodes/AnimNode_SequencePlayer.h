#pragma once

#include "Animation/AnimNodes/AnimNode_AssetPlayerBase.h"

class UAnimSequence;

struct FAnimNode_SequencePlayer : public FAnimNode_AssetPlayerBase
{
	UAnimSequence* Sequence = nullptr;
	float CurrentTime = 0.f;
	float PreviousTime = 0.f;

	void Initialize() override
	{
		CurrentTime = 0.f;
		PreviousTime = 0.f;
	}

	void Update(FAnimationUpdateContext& UpdateContext) override;

	void Evaluate(FPoseContext& OutPoseContext) override;

	float GetCurrentAssetDuration() override;
	
	float GetNormalizedTime() const override;
};

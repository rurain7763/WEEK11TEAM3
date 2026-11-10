#pragma once

#include "Animation/AnimNodes/AnimNode_AssetPlayerBase.h"
#include "Animation/BlendSpace.h"

struct FAnimNode_BlendSpacePlayer : public FAnimNode_AssetPlayerBase
{
public:
	void UpdateSampleCache();

	void Initialize() override
	{
		UpdateSampleCache();
		NormalizedTime = 0.f;
	}

	void Update(FAnimationUpdateContext& UpdateContext) override;

	void Evaluate(FPoseContext& OutPoseContext) override;

	inline void SetBlendSpace(UBlendSpace* InBlendSpace)
	{
		BlendSpace = InBlendSpace;
	}

	inline void SetX(float InX)
	{
		X = InX;
	}

	inline void SetY(float InY)
	{
		Y = InY;
	}

	// NOTE: 가장 높은 가중치를 가진 샘플의 Duration 반환
	float GetCurrentAssetDuration() override;

	virtual float GetNormalizedTime() const override;

private:
	inline FVector GetPosition() const
	{
		return FVector(X, Y, 0.f);
	}

private:
	UBlendSpace* BlendSpace = nullptr;
	float X = 0.f;
	float Y = 0.f;

	FBlendSampleData CachedSamples[MaxBlendSampleCount];
	int32 CachedSampleCount = 0;
	FBlendSampleData* MaxSample = nullptr;
	float NormalizedTimeSpeed = 0.f;

	float NormalizedTime = 0.f;
};

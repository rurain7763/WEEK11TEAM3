#pragma once

#include "Animation/AnimNodes/AnimNode_Base.h"

struct FAnimNode_AssetPlayerBase : public FAnimNode_Base
{
public:
	int32 ID = 0;
	bool bLooping = true;
	float Speed = 1.f;

	FAnimNode_AssetPlayerBase()
	{
		ID = NextID++;
	}

	virtual ~FAnimNode_AssetPlayerBase() override = default;

	virtual float GetCurrentAssetDuration()
	{
		return 0.f;
	}

	float GetSpeed() const override { return Speed; }

protected:
	bool bExtractRootMotion = true;

private:
	inline static int32 NextID = 0;
};

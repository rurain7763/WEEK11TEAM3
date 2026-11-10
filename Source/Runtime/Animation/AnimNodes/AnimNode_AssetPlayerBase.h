#pragma once

#include "Animation/AnimNodes/AnimNode_Base.h"

struct FAnimNode_AssetPlayerBase : public FAnimNode_Base
{
	bool bLooping = true;
	float Speed = 1.f;

	virtual float GetCurrentAssetDuration()
	{
		return 0.f;
	}
};

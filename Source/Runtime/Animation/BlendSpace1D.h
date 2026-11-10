#pragma once

#include "Animation/BlendSpace.h"

class UBlendSpace1D : public UBlendSpace
{
	DECLARE_CLASS(UBlendSpace1D, UBlendSpace)

public:
	virtual bool GetSamples(const FVector& Position, FBlendSampleData* OutSampleDatas, int32& OutSampleCount) override;
};

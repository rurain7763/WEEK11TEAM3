#pragma once

#include "Animation/AnimationAsset.h"
#include "Containers/Array.h"
#include "Math/Vector.h"

class UAnimSequence;

constexpr int32 MaxBlendSampleCount = 3;

struct FBlendSample
{
	UAnimSequence* Animation;
	FVector SampleValue;
};

struct FBlendSampleData
{
	UAnimSequence* Animation;
	float Weight;
};

class UBlendSpace : public UAnimationAsset
{
	DECLARE_CLASS(UBlendSpace, UAnimationAsset)

public:
	virtual void Serialize(FStructuredArchive::FRecord Record) override;

	void AddSample(UAnimSequence* Animation, const FVector& SampleValue)
	{
		Samples.Emplace(FBlendSample{ Animation, SampleValue });
	}

	virtual bool GetSamples(const FVector& Position, FBlendSampleData* OutSampleDatas, int32& OutSampleCount)
	{
		OutSampleCount = 0;
		return false;
	}

	const TArray<FBlendSample>& GetBlendSamples() const
	{
		return Samples;
	}

	void SetBlendSamples(const TArray<FBlendSample>& InSamples)
	{
		Samples = InSamples;
	}

	void SortBlendSamples();

protected:
	TArray<FBlendSample> Samples;
};

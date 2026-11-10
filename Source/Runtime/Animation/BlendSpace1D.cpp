#include "EnginePCH.h"
#include "Animation/BlendSpace1D.h"

bool UBlendSpace1D::GetSamples(const FVector& Position, FBlendSampleData* OutSampleDatas, int32& OutSampleCount)
{
	if (Samples.IsEmpty())
	{
		return false;
	}

	for (int32 i = 0; i < Samples.Num() - 1; i++)
	{
		FBlendSample& A = Samples[i];
		FBlendSample& B = Samples[i + 1];

		if (Position.X >= A.SampleValue.X && Position.X <= B.SampleValue.X)
		{
			float Alpha = (Position.X - A.SampleValue.X) / (B.SampleValue.X - A.SampleValue.X);

			OutSampleCount = 2;
			OutSampleDatas[0] = FBlendSampleData{ A.Animation, 1.f - Alpha };
			OutSampleDatas[1] = FBlendSampleData{ B.Animation, Alpha };
			return true;
		}
	}

	FBlendSample& Front = Samples.Front();
	FBlendSample& Last = Samples.Last();

	if (Position.X <= Front.SampleValue.X)
	{
		OutSampleCount = 1;
		OutSampleDatas[0] = FBlendSampleData{ Front.Animation, 1.f };
		return true;
	}
	else if (Position.X >= Last.SampleValue.X)
	{
		OutSampleCount = 1;
		OutSampleDatas[0] = FBlendSampleData{ Last.Animation, 1.f };
		return true;
	}

	return false;
}

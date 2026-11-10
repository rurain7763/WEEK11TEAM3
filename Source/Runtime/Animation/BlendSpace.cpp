#include "EnginePCH.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"
#include "Asset/AssetManager.h"
#include "Serialization/MathSerialization.h"
#include <algorithm>

void UBlendSpace::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	FArchive& Archive = Record.GetUnderlyingArchive();

	int32 SamplesNum = Samples.Num();
	FStructuredArchive::FArray SampleArray = Record.EnterArray("Samples", SamplesNum);
	if (Archive.IsLoading())
	{
		Samples.SetNum(SamplesNum);
	}

	for (int32 i = 0; i < SamplesNum; i++)
	{
		FBlendSample& Sample = Samples[i];

		FStructuredArchiveRecord ElementRecord = SampleArray.EnterElement().EnterRecord();

		FString SequencePath = Sample.Animation ? Sample.Animation->GetPath() : "";
		ElementRecord << SA_VALUE("Animation", SequencePath);
		if (Archive.IsLoading())
		{
			Sample.Animation = UAssetManager::GetAssetByPath<UAnimSequence>(SequencePath);
		}

		ElementRecord << SA_VALUE("SampleValue", Sample.SampleValue);
	}
}

void UBlendSpace::SortBlendSamples()
{
	std::sort(Samples.begin(), Samples.end(), [](const FBlendSample& A, const FBlendSample& B) {
		if (A.SampleValue.X != B.SampleValue.X)
		{
			return A.SampleValue.X < B.SampleValue.X;
		}

		if (A.SampleValue.Y != B.SampleValue.Y)
		{
			return A.SampleValue.Y < B.SampleValue.Y;
		}

		return A.SampleValue.Z < B.SampleValue.Z;
	});
}

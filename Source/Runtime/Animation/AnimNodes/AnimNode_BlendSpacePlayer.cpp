#include "EnginePCH.h"
#include "Animation/AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationRuntime.h"
#include <cmath>

void FAnimNode_BlendSpacePlayer::UpdateSampleCache()
{
	CachedSampleCount = 0;
	MaxSample = nullptr;
	NormalizedTimeSpeed = 0.f;
	if (BlendSpace->GetSamples(GetPosition(), CachedSamples, CachedSampleCount))
	{
		for (int32 i = 0; i < CachedSampleCount; i++)
		{
			FBlendSampleData& SampleData = CachedSamples[i];

			NormalizedTimeSpeed += 1.f / SampleData.Animation->GetDuration() * SampleData.Weight;
			if (!MaxSample || SampleData.Weight > MaxSample->Weight)
			{
				MaxSample = &SampleData;
			}
		}
	}
}

void FAnimNode_BlendSpacePlayer::Update(FAnimationUpdateContext& UpdateContext)
{
	UpdateSampleCache();

	float StartNormalizedTime = NormalizedTime;
	float NormalizedDeltaTime = UpdateContext.DeltaTime * Speed * NormalizedTimeSpeed;

	NormalizedTime += NormalizedDeltaTime;

	float WeightSum = 0.f;
	for (int32 i = 0; i < CachedSampleCount; i++)
	{
		FBlendSampleData& SampleData = CachedSamples[i];
		if (SampleData.Weight <= 0.f)
		{
			continue;
		}

		WeightSum += SampleData.Weight;

		FTransform RootMotion = FTransform::Identity;
		if (SampleData.Animation->HasRootMotion())
		{
			float Duration = SampleData.Animation->GetDuration();
			float DeltaTime = Duration * NormalizedDeltaTime;

			FAnimExtractContext Context;
			Context.CurrentTime = Duration * StartNormalizedTime;
			Context.bLooping = bLooping;

			RootMotion = SampleData.Animation->ExtractRootMotion(Context, Duration, DeltaTime);
		}

		float Alpha = SampleData.Weight / WeightSum;
		UpdateContext.bHasRootMotion |= SampleData.Animation->HasRootMotion();
		UpdateContext.RootMotion = FTransform::Blend(UpdateContext.RootMotion, RootMotion, Alpha);
	}

	if (NormalizedTime >= 1.f)
	{
		if (bLooping)
		{
			while (NormalizedTime > 1.f)
			{
				NormalizedTime = NormalizedTime - 1.f;
			}
		}
		else
		{
			NormalizedTime = 1.f;
		}
	}
	else if (NormalizedTime < 0.f)
	{
		if (bLooping)
		{
			NormalizedTime = 1.f + std::fmod(NormalizedTime, 1.f);
		}
		else
		{
			NormalizedTime = 0.f;
		}
	}
}

void FAnimNode_BlendSpacePlayer::Evaluate(FPoseContext& OutPoseContext)
{
	if (CachedSampleCount == 1)
	{
		const FBlendSampleData& SampleData = CachedSamples[0];

		FAnimExtractContext ExtractionContext;
		ExtractionContext.CurrentTime = SampleData.Animation->GetDuration() * NormalizedTime;
		ExtractionContext.bLooping = bLooping;
		ExtractionContext.bExtractRootMotion = SampleData.Animation->HasRootMotion();

		SampleData.Animation->GetAnimationPose(ExtractionContext, OutPoseContext);
	}
	else if (CachedSampleCount == 2)
	{
		const FBlendSampleData& SampleDataA = CachedSamples[0];
		const FBlendSampleData& SampleDataB = CachedSamples[1];

		FAnimExtractContext ExtractionContextA;
		ExtractionContextA.CurrentTime = SampleDataA.Animation->GetDuration() * NormalizedTime;
		ExtractionContextA.bLooping = bLooping;
		ExtractionContextA.bExtractRootMotion = SampleDataA.Animation->HasRootMotion();

		FPoseContext PoseA;
		SampleDataA.Animation->GetAnimationPose(ExtractionContextA, PoseA);

		FAnimExtractContext ExtractionContextB;
		ExtractionContextB.CurrentTime = SampleDataB.Animation->GetDuration() * NormalizedTime;
		ExtractionContextB.bLooping = bLooping;
		ExtractionContextB.bExtractRootMotion = SampleDataB.Animation->HasRootMotion();

		FPoseContext PoseB;
		SampleDataB.Animation->GetAnimationPose(ExtractionContextB, PoseB);

		FAnimationRuntime::BlendTwoPosesTogether(PoseA, PoseB, SampleDataB.Weight, OutPoseContext);
	}
}

float FAnimNode_BlendSpacePlayer::GetCurrentAssetDuration()
{
	if (!BlendSpace || !MaxSample)
	{
		return 0.f;
	}

	return MaxSample->Animation->GetDuration();
}

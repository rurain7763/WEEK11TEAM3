#include "EnginePCH.h"
#include "Animation/AnimNodes/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include <cmath>

void FAnimNode_SequencePlayer::Update(FAnimationUpdateContext& UpdateContext)
{
	const float Duration = GetCurrentAssetDuration();
	if (Duration <= 0.f)
	{
		return;
	}

	float DeltaTime = UpdateContext.DeltaTime * Speed;

	PreviousTime = CurrentTime;
	CurrentTime += DeltaTime;

	FAnimNotifyContext NotifyContext;
	NotifyContext.SourcePlayerID = ID;
	Sequence->GetAnimNotifies(Sequence->GetDuration(), bLooping, PreviousTime, DeltaTime, NotifyContext);
	UpdateContext.EventQueue->Append(NotifyContext.CollectedNotifies);

	if (Sequence->HasRootMotion())
	{
		FAnimExtractContext ExtractionContext;
		ExtractionContext.CurrentTime = PreviousTime;
		ExtractionContext.bLooping = bLooping;

		UpdateContext.bHasRootMotion |= true;
		UpdateContext.RootMotion = Sequence->ExtractRootMotion(ExtractionContext, Sequence->GetDuration(), DeltaTime);
	}

	if (CurrentTime >= Duration)
	{
		if (bLooping)
		{
			while (CurrentTime > Duration)
			{
				CurrentTime = CurrentTime - Duration;
			}
		}
		else
		{
			CurrentTime = Duration;
		}
	}
	else if (CurrentTime < 0.f)
	{
		if (bLooping && Duration > 0.f)
		{
			const float Remainder = std::fmod(CurrentTime, Duration);
			CurrentTime = Remainder == 0.f ? 0.f : Duration + Remainder;
		}
		else
		{
			CurrentTime = 0.f;
		}
	}
}

void FAnimNode_SequencePlayer::Evaluate(FPoseContext& OutPoseContext)
{
	if (!Sequence)
	{
		return;
	}

	FAnimExtractContext ExtractionContext;
	ExtractionContext.CurrentTime = CurrentTime;
	ExtractionContext.bLooping = bLooping;
	ExtractionContext.bExtractRootMotion = Sequence->HasRootMotion();

	Sequence->GetAnimationPose(ExtractionContext, OutPoseContext);
}

float FAnimNode_SequencePlayer::GetCurrentAssetDuration()
{
	if (!Sequence)
	{
		return 0.f;
	}

	return Sequence->GetDuration();
}

float FAnimNode_SequencePlayer::GetNormalizedTime() const
{
	if (!Sequence)
	{
		return 0.f;
	}

	return CurrentTime / Sequence->GetDuration();
}

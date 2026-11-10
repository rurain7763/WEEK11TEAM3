#include "EnginePCH.h"
#include "Animation/AnimNodes/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"

void FAnimNode_SequencePlayer::Update(FAnimationUpdateContext& UpdateContext)
{
	float DeltaTime = UpdateContext.DeltaTime * Speed;

	PreviousTime = CurrentTime;
	CurrentTime += DeltaTime;

	Sequence->GetAnimNotifies(Sequence->GetDuration(), bLooping, PreviousTime, DeltaTime, *UpdateContext.EventQueue);

	if (Sequence->HasRootMotion())
	{
		FAnimExtractContext ExtractionContext;
		ExtractionContext.CurrentTime = PreviousTime;
		ExtractionContext.bLooping = bLooping;

		UpdateContext.bHasRootMotion |= true;
		UpdateContext.RootMotion = Sequence->ExtractRootMotion(ExtractionContext, Sequence->GetDuration(), DeltaTime);
	}

	float Duration = GetCurrentAssetDuration();
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

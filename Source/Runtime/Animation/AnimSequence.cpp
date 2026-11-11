#include "EnginePCH.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimDataModel.h"
#include "Asset/ModelData.h"
#include "Core/Misc/FrameTime.h"
#include <stdexcept>

void UAnimSequence::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	Record << SA_VALUE("EnableRootMotion", bEnableRootMotion);
}

void UAnimSequence::GetAnimationPose(const FAnimExtractContext& ExtractionContext, FPoseContext& PoseData) const
{
	UAnimDataModel* Data = GetDataModel();

	const auto& Tracks = Data->GetBoneAnimationTracks();

	int32 BoneCount = Tracks.Num();

	PoseData.BoneCount = BoneCount;
	PoseData.BoneTranslations.SetNum(BoneCount);
	PoseData.BoneRotations.SetNum(BoneCount);
	PoseData.BoneScales.SetNum(BoneCount);

	const FBoneAnimationTrack* RootTrack = nullptr;
	if (ExtractionContext.bExtractRootMotion)
	{
		RootTrack = Data->GetRootBoneTrack();
	}

	FFrameTime CurrentFrameTime = FFrameTime::FromDecimal(ExtractionContext.CurrentTime * Data->GetSampleRate());
	for (int32 i = 0; i < Tracks.Num(); i++)
	{
		const FBoneAnimationTrack& Track = Tracks[i];

		if (&Track != RootTrack)
		{
			GetPoseFromTrack(CurrentFrameTime, Track, PoseData.BoneTranslations[Track.BoneIndex], PoseData.BoneRotations[Track.BoneIndex], PoseData.BoneScales[Track.BoneIndex]);
		}
		else
		{
			PoseData.BoneTranslations[Track.BoneIndex] = Track.PosKeys[0];
			PoseData.BoneRotations[Track.BoneIndex] = Track.RotKeys[0];
			PoseData.BoneScales[Track.BoneIndex] = Track.ScaleKeys[0];
		}
	}
}

FTransform UAnimSequence::ExtractRootMotion(const FAnimExtractContext& ExtractionContext, float Duration, float DeltaTime)
{
	FTransform Combined = FTransform::Identity;
	float StartTime = ExtractionContext.CurrentTime;

	if (Duration <= 0.f)
	{
		return Combined;
	}
	if (DeltaTime > 0.f)
	{
		while (DeltaTime > 0.f)
		{
			float NextTime = StartTime + DeltaTime;

			FTransform DeltaTransform;
			float DeltaStep = 0.f;
			if (NextTime > Duration)
			{
				DeltaTransform = ExtractRootMotionFromRange(ExtractionContext, StartTime, Duration);

				if (ExtractionContext.bLooping)
				{
					DeltaStep = Duration - StartTime;
					StartTime = 0.f;
				}
				else
				{
					DeltaStep = DeltaTime;
				}
			}
			else
			{
				DeltaTransform = ExtractRootMotionFromRange(ExtractionContext, StartTime, NextTime);
				DeltaStep = DeltaTime;
			}

			Combined = DeltaTransform * Combined;

			DeltaTime -= DeltaStep;
		}
	}
	else if (DeltaTime < 0.f)
	{
		while (DeltaTime < 0.f)
		{
			const float NextTime = StartTime + DeltaTime;
			const bool bCrossedStart = NextTime < 0.f;
			const float EndTime = bCrossedStart ? 0.f : NextTime;

			if (EndTime < StartTime)
			{
				const FTransform DeltaTransform = ExtractRootMotionFromRange(ExtractionContext, StartTime, EndTime);
				Combined = DeltaTransform * Combined;
			}

			if (!bCrossedStart || !ExtractionContext.bLooping)
			{
				return Combined;
			}

			DeltaTime += StartTime;
			StartTime = Duration;
		}
	}

	return Combined;
}

FTransform UAnimSequence::ExtractRootMotionFromRange(const FAnimExtractContext& ExtractionContext, float StartTime, float EndTime) const
{
	FAnimExtractContext StartExtractionCtx = ExtractionContext;
	StartExtractionCtx.CurrentTime = StartTime;
	FTransform StartTransform = ExtractRootTrackTransform(StartExtractionCtx);

	FAnimExtractContext EndExtractionCtx = ExtractionContext;
	EndExtractionCtx.CurrentTime = EndTime;
	FTransform EndTransform = ExtractRootTrackTransform(EndExtractionCtx);

	return EndTransform.GetRelativeTransform(StartTransform);
}

FTransform UAnimSequence::ExtractRootTrackTransform(const FAnimExtractContext& ExtractionContext) const
{
	UAnimDataModel* Data = GetDataModel();

	const FBoneAnimationTrack* Track = Data->GetRootBoneTrack();
	if (!Track)
	{
		return FTransform::Identity;
	}

	FFrameTime CurrentFrameTime = FFrameTime::FromDecimal(ExtractionContext.CurrentTime * Data->GetSampleRate());

	FVector Position;
	FQuat Rotation;
	FVector Scale;
	GetPoseFromTrack(CurrentFrameTime, *Track, Position, Rotation, Scale);

	return FTransform(Position, Scale, Rotation);
}

void UAnimSequence::GetPoseFromTrack(const FFrameTime& FrameTime, const FBoneAnimationTrack& Track, FVector& Position, FQuat& Rotation, FVector& Scale) const
{
	int32 KeyIndexA = FrameTime.FrameNumber;
	int32 KeyIndexB = FrameTime.FrameNumber + 1;

	if (KeyIndexA < 0 || KeyIndexA >= Track.KeyCount)
	{
		throw std::runtime_error("Something goes wrong!");
	}

	const FVector& PositionA = Track.PosKeys[KeyIndexA];
	const FQuat& RotationA = Track.RotKeys[KeyIndexA];
	const FVector& ScaleA = Track.ScaleKeys[KeyIndexA];

	if (KeyIndexB >= Track.KeyCount)
	{
		Position = PositionA;
		Rotation = RotationA;
		Scale = ScaleA;
		return;
	}

	const FVector& PositionB = Track.PosKeys[KeyIndexB];
	const FQuat& RotationB = Track.RotKeys[KeyIndexB];
	const FVector& ScaleB = Track.ScaleKeys[KeyIndexB];

	// TODO: 항상 lerp, slerp 보관만 일단 지원
	Position = FMath::Lerp(PositionA, PositionB, FrameTime.SubFrame);
	Rotation = FQuat::Slerp(RotationA, RotationB, FrameTime.SubFrame);
	Scale = FMath::Lerp(ScaleA, ScaleB, FrameTime.SubFrame);
}

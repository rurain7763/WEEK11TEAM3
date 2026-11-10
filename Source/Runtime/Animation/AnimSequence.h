#pragma once

#include "Animation/AnimSequenceBase.h"

struct FBoneAnimationTrack;
struct FFrameTime;

class UAnimSequence : public UAnimSequenceBase
{
	DECLARE_CLASS(UAnimSequence, UAnimSequenceBase)

public:
	void GetAnimationPose(const FAnimExtractContext& ExtractionContext, FPoseContext& PoseData) const override;

	FTransform ExtractRootMotion(const FAnimExtractContext& ExtractionContext, float Duration, float DeltaTime);

	FTransform ExtractRootMotionFromRange(const FAnimExtractContext& ExtractionContext, float StartTime, float EndTime) const override;

	FTransform ExtractRootTrackTransform(const FAnimExtractContext& ExtractionContext) const override;

	bool HasRootMotion() const
	{
		return bEnableRootMotion;
	}

private:
	void GetPoseFromTrack(const FFrameTime& FrameTime, const FBoneAnimationTrack& Track, FVector& Position, FQuat& Rotation, FVector& Scale) const;

private:
	bool bEnableRootMotion = false;
};

#pragma once

#include "Animation/AnimationAsset.h"
#include "Animation/AnimationTypes.h"
#include "Animation/AnimNotify.h"

class UAnimDataModel;

class UAnimSequenceBase : public UAnimationAsset
{
	DECLARE_CLASS(UAnimSequenceBase, UAnimationAsset)

public:
	virtual ~UAnimSequenceBase() = default;

	virtual void Initialize(UAnimDataModel* InDataModel)
	{
		DataModel = InDataModel;
	}

	virtual float GetDuration() const override;

	virtual void GetAnimationPose(const FAnimExtractContext& ExtractionContext, FPoseContext& PoseData) const = 0;
	virtual FTransform ExtractRootMotion(const FAnimExtractContext& ExtractionContext, float Duration, float DeltaTime) = 0;
	virtual FTransform ExtractRootMotionFromRange(const FAnimExtractContext& ExtractionContext, float StartTime, float EndTime) const = 0;
	virtual FTransform ExtractRootTrackTransform(const FAnimExtractContext& ExtractionContext) const = 0;

	virtual bool HasRootMotion() const
	{
		return false;
	}

	void GetAnimNotifies(float Duration, bool bLooping, float StartTime, float DeltaTime, TArray<FAnimNotifyEvent*>& OutEvents);

	void GetAnimNotifiesFromPositions(float PreviousPosition, float CurrentPosition, bool bIncludeStart, TArray<FAnimNotifyEvent*>& OutEvents);

	UAnimDataModel* GetDataModel() const
	{
		return DataModel;
	}

private:
	UAnimDataModel* DataModel = nullptr;
	TArray<FAnimNotifyEvent> Notifies;
};

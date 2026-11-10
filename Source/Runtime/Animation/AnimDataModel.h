#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Asset/ModelData.h"

class UAnimDataModel : public UObject
{
	DECLARE_CLASS(UAnimDataModel, UObject)

public:
	void Initialize(const FModelBoneAnimation& BoneAnimation);

	const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const
	{
		return BoneAnimationTracks;
	}

	const FBoneAnimationTrack* GetRootBoneTrack() const
	{
		return RootBoneTrackIndex != -1 ? &BoneAnimationTracks[RootBoneTrackIndex] : nullptr;
	}

	float GetDuration() const
	{
		return Duration;
	}

	int32 GetSampleRate() const
	{
		return SampleRate;
	}

private:
	TArray<FBoneAnimationTrack> BoneAnimationTracks;
	int32 RootBoneTrackIndex = -1;
	float Duration = 0.f;
	int32 SampleRate = 0;
};

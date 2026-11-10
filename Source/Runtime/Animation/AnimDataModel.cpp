#include "EnginePCH.h"
#include "Animation/AnimDataModel.h"

void UAnimDataModel::Initialize(const FModelBoneAnimation& BoneAnimation)
{
	BoneAnimationTracks = BoneAnimation.Tracks;

	RootBoneTrackIndex = -1;
	for (int32 i = 0; i < BoneAnimation.Tracks.Num(); i++)
	{
		if (BoneAnimation.Tracks[i].BoneIndex == 0)
		{
			RootBoneTrackIndex = i;
			break;
		}
	}

	Duration = BoneAnimation.Duration;
	SampleRate = BoneAnimation.SampleRate;
}

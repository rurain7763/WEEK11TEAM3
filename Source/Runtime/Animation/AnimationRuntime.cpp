#include "EnginePCH.h"
#include "Animation/AnimationRuntime.h"

void FAnimationRuntime::BlendTwoPosesTogether(const FPoseContext& PoseA, const FPoseContext& PoseB, float T, FPoseContext& OutPose)
{
	int32 MinBoneCount = FMath::Min(PoseA.BoneCount, PoseB.BoneCount);

	OutPose.BoneCount = MinBoneCount;
	OutPose.BoneTranslations.SetNum(MinBoneCount);
	OutPose.BoneRotations.SetNum(MinBoneCount);
	OutPose.BoneScales.SetNum(MinBoneCount);

	for (int32 i = 0; i < MinBoneCount; i++)
	{
		OutPose.BoneTranslations[i] = FMath::Lerp(PoseA.BoneTranslations[i], PoseB.BoneTranslations[i], T);
		OutPose.BoneRotations[i] = FQuat::Slerp(PoseA.BoneRotations[i], PoseB.BoneRotations[i], T);
		OutPose.BoneScales[i] = FMath::Lerp(PoseA.BoneScales[i], PoseB.BoneScales[i], T);
	}
}

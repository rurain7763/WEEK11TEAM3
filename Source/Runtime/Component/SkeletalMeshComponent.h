#pragma once

#include "Component/SkinnedMeshComponent.h"
#include "Animation/Graph/AnimGraph.h"

class UAnimationAsset;
class UAnimInstance;

enum class EAnimationMode
{
	AnimationSingleNode
};

class USkeletalMeshComponent : public USkinnedMeshComponent
{
	DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

	REFLECT_START(ThisClass)
		PROPERTY(AnimGraph)
		PROPERTY(bExtractRootMotion)
	REFLECT_END()

public:
	USkeletalMeshComponent();

	void SetAnimationMode(EAnimationMode AnimationMode);
	void SetAnimation(UAnimationAsset* NewAnimAsset);
	void PlayAnimation(UAnimationAsset* NewAnimAsset, bool bLooping, float PlayRate = 1.0f);

	bool HasRootMotion() const;
	bool ShouldExtractRootMotion() const { return bExtractRootMotion; }
	FTransform ConsumeRootMotion();
	FTransform ConvertLocalRootMotionToWorld(const FTransform& InLocal);

	UAnimInstance* GetAnimInstance() const { return AnimInstance; }

	void BeginPlay() override;
	void EndPlay() override;
	void TickComponent(float DeltaTime)	override;

	virtual const FModelMeshData* GetMeshData() const override;

	void SubmitToRenderQueue(FRenderQueue& RenderQueue) override;

	FPrimitiveSceneProxy* CreateSceneProxy() override;

	void SetBoneWeightVisualization(bool bEnabled, int32 BoneIndex);
	bool IsBoneWeightVisualizationEnabled() const { return bBoneWeightVisualizationEnabled; }
	int32 GetHeatmapBoneIndex() const { return HeatmapBoneIndex; }

private:
	void Play(bool bLooping);

private:
	UAnimGraph* AnimGraph = nullptr;
	UAnimInstance* AnimInstance = nullptr;
	bool bExtractRootMotion = true;
	bool bBoneWeightVisualizationEnabled = false;
	int32 HeatmapBoneIndex = -1;
};

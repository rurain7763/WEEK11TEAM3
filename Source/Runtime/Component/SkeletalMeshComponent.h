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
	REFLECT_END()

public:
	USkeletalMeshComponent();

	void SetAnimationMode(EAnimationMode AnimationMode);
	void SetAnimation(UAnimationAsset* NewAnimAsset);
	void PlayAnimation(UAnimationAsset* NewAnimAsset, bool bLooping);

	bool HasRootMotion() const;
	FTransform ConsumeRootMotion();
	FTransform ConvertLocalRootMotionToWorld(const FTransform& InLocal);

	UAnimInstance* GetAnimInstance() const { return AnimInstance; }

	void BeginPlay() override;
	void EndPlay() override;
	void TickComponent(float DeltaTime)	override;

	virtual const FModelMeshData* GetMeshData() const override;

	void SubmitToRenderQueue(FRenderQueue& RenderQueue) override;

	FPrimitiveSceneProxy* CreateSceneProxy() override;

private:
	void Play(bool bLooping);

private:
	UAnimGraph* AnimGraph = nullptr;
	UAnimInstance* AnimInstance = nullptr;
};

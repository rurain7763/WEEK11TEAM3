#include "EnginePCH.h"
#include "Animation/Graph/AnimGraph.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Component/SkeletalMeshComponent.h"

#include "Engine/PrimitiveSceneProxy.h"
#include "Render/RenderPacket.h"
#include "Animation/AnimSequence.h"

USkeletalMeshComponent::USkeletalMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USkeletalMeshComponent::SetAnimationMode(EAnimationMode AnimationMode)
{
	if (AnimationMode == EAnimationMode::AnimationSingleNode)
	{
		UAnimSingleNodeInstance* SingleNode = Cast<UAnimSingleNodeInstance>(AnimInstance);
		if (!SingleNode)
		{
			if (AnimInstance)
			{
				delete AnimInstance;
			}

			SingleNode = NewObject<UAnimSingleNodeInstance>();
			AnimInstance = SingleNode;
		}
		SingleNode->SetSkeletalMeshComponent(this);
	}
}

void USkeletalMeshComponent::SetAnimation(UAnimationAsset* NewAnimAsset)
{
	if (UAnimSingleNodeInstance* SingleNode = Cast<UAnimSingleNodeInstance>(AnimInstance))
	{
		SingleNode->SetAnimationAsset(NewAnimAsset);
	}
}

void USkeletalMeshComponent::PlayAnimation(UAnimationAsset* NewAnimAsset, bool bLooping, float PlayRate)
{
	if (!Cast<UAnimSequence>(NewAnimAsset))
	{
		return;
	}
	SetAnimationMode(EAnimationMode::AnimationSingleNode);

	UAnimSingleNodeInstance* SingleNode = Cast<UAnimSingleNodeInstance>(AnimInstance);

	if (!SingleNode)
	{
		return;
	}
	SetAnimation(NewAnimAsset);
	Play(bLooping);
	SingleNode->SetPlayRate(PlayRate);
	SingleNode->NativeInitializeAnimation();
}

bool USkeletalMeshComponent::HasRootMotion() const
{
	if (!AnimInstance)
	{
		return false;
	}

	return AnimInstance->HasRootMotion();
}

FTransform USkeletalMeshComponent::ConsumeRootMotion()
{
	if (!AnimInstance)
	{
		return FTransform::Identity;
	}

	FTransform RootMotion = AnimInstance->ConsumeRootMotion();

	FMatrix RootToModelMatrix = FMatrix::Identity;
	if (const FModelBone* RootBone = Skeleton->GetRootBone())
	{
		RootToModelMatrix = NodeGlobalTransforms[RootBone->NodeIndex];
	}

	FQuat RootToModelRotation = FQuat::MakeFromMatrix(RootToModelMatrix.GetMatrixWithoutScale());
	FQuat ModelRootMotionRotation = RootToModelRotation * RootMotion.GetOrientation() * RootToModelRotation.UnitInverse();

	RootMotion.Location = RootToModelMatrix.TransformVector(RootMotion.Location);
	RootMotion.Rotation = ModelRootMotionRotation.ToFRotator();

	return RootMotion;
}

FTransform USkeletalMeshComponent::ConvertLocalRootMotionToWorld(const FTransform& InLocal)
{
	FMatrix WorldMatrix = GetWorldMatrix();

	FQuat ModelToWorldRotation = FQuat::MakeFromMatrix(WorldMatrix.GetMatrixWithoutScale());
	FQuat WorldRootMotionRotation = ModelToWorldRotation * InLocal.GetOrientation() * ModelToWorldRotation.UnitInverse();

	FTransform Converted = InLocal;
	Converted.Location = WorldMatrix.TransformVector(Converted.Location);
	Converted.Rotation = WorldRootMotionRotation.ToFRotator();

	return Converted;
}

void USkeletalMeshComponent::Play(bool bLooping)
{
	if (UAnimSingleNodeInstance* SingleNode = Cast<UAnimSingleNodeInstance>(AnimInstance))
	{
		SingleNode->SetLoop(bLooping);
	}
}

void USkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AnimGraph)
	{
		AnimInstance = AnimGraph->CreateAnimInstance();
		AnimInstance->SetSkeletalMeshComponent(this);
		AnimInstance->NativeInitializeAnimation();
	}
}

void USkeletalMeshComponent::EndPlay()
{
	Super::EndPlay();

	if (AnimInstance)
	{
		delete AnimInstance;
		AnimInstance = nullptr;
	}
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->NativeUpdateAnimation(DeltaTime);

	FPoseContext PoseData;
	AnimInstance->Evaluate(PoseData);
	UpdatePose(PoseData);

	UpdateBoneMatrices();

	AnimInstance->DispatchQueuedAnimEvents();
}

const FModelMeshData* USkeletalMeshComponent::GetMeshData() const
{
	if (!SkeletalMesh)
	{
		return nullptr;
	}

	return &SkeletalMesh->GetMeshData();
}

void USkeletalMeshComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue)
{
	const FPrimitiveSceneProxy* Proxy = GetSceneProxy();
	if (!SkeletalMesh || !Proxy)
	{
		return;
	}

	const FMeshRenderState* State = Proxy->GetRenderState();
	if (!State)
	{
		return;
	}

	const int32 LODIndex = 0;

	const FCachedMeshLOD& LOD = State->LODs[LODIndex];
	for (uint32 I = 0; I < LOD.NumSections; ++I)
	{
		const FCachedMeshSection& Section = State->Sections[LOD.FirstSection + I];

		FRenderPacket& Packet = RenderQueue.AddDefaulted_GetRef();
		Packet.Proxy = Proxy;
		Packet.Geometry.VertexBuffers[0] = LOD.VertexBuffers[0];
		Packet.Geometry.VertexBuffers[1] = LOD.VertexBuffers[1];
		// CPU 스킨이면 1개(스키닝된 정점), GPU 스킨이면 2개(원본 정점 + 가중치)
		Packet.Geometry.VertexBufferCount = LOD.VertexBufferCount;
		Packet.Geometry.IndexBuffer = LOD.IndexBuffer;
		Packet.Geometry.StartIndex = Section.StartIndex;
		Packet.Geometry.IndexCount = Section.IndexCount;
		Packet.Material = Section.Material;
	}
}

FPrimitiveSceneProxy* USkeletalMeshComponent::CreateSceneProxy()
{
	return new FSkeletalMeshSceneProxy(this);
}

void USkeletalMeshComponent::SetBoneWeightVisualization(bool bEnabled, int32 BoneIndex)
{
	if (bBoneWeightVisualizationEnabled == bEnabled && HeatmapBoneIndex == BoneIndex)
	{
		return;
	}

	bBoneWeightVisualizationEnabled = bEnabled;
	HeatmapBoneIndex = BoneIndex;
	MarkRenderStateDirty();
}

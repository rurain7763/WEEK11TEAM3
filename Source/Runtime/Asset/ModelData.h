#pragma once

#include "Core/Types.h"
#include "Core/EngineString.h"
#include "Core/Misc/FrameTime.h"
#include "Containers/Array.h"
#include "Math/Matrix.h"
#include "Render/StaticMeshData.h"
#include "Math/CurveInterpMode.h"

constexpr int32 MaxBoneInfluenceCount = 4;

struct FModelNode
{
	FString Name;
	int32 ParentNodeIndex = -1;
	FMatrix LocalTransform = FMatrix::Identity;
};

struct FModelBone
{
	int32 NodeIndex;
	int32 ParentBoneIndex;
	FMatrix InverseBindTransform;
};

struct FModelMesh
{
	int32 NodeIndex;
	int32 FirstSection = -1;
	int32 SectionCount = 0;
};

struct FModelVertexInfluences
{
	int32 BoneIndices[MaxBoneInfluenceCount] = { -1, -1, -1, -1 };
	float Weights[MaxBoneInfluenceCount] = { 0.f, 0.f, 0.f, 0.f };
	int32 InfluenceCount = 0;
};

struct FBoneAnimationTrack
{
	int32 BoneIndex = -1;
	int32 KeyCount = 0;
	TArray<FVector> PosKeys;
	TArray<FQuat> RotKeys;
	TArray<FVector> ScaleKeys;
};

struct FModelBoneAnimation
{
	FString Name;
	float Duration; // 단위는 초
	int32 SampleRate; // 초당 샘플 수
	TArray<FBoneAnimationTrack> Tracks;
};

struct FModelData
{
	bool IsSkeletal;
	FModelMeshData MeshData;
	TArray<FModelVertexInfluences> VertexInfluencesList;
	TArray<FModelNode> Nodes;
	TArray<FModelBone> Bones;
	TArray<FModelMesh> Meshs;
	TArray<FModelBoneAnimation> Animations;
};


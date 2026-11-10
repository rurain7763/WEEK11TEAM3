#pragma once

#include "LightweightStats.h"

DECLARE_CYCLE_STAT("Skinning CPU Vertices", STAT_SkinningCPUVertices);
DECLARE_CYCLE_STAT("Skinning CPU VB Upload", STAT_SkinningCPUUpload);
DECLARE_CYCLE_STAT("Skinning GPU Bone Upload", STAT_SkinningGPUBoneUpload);

struct FSkinningFrameStats
{
	int32 CPUSkinnedMeshes = 0;
	int32 CPUSkinnedVertices = 0;
	int64 CPUUploadBytes = 0;

	int32 GPUSkinnedMeshes = 0;
	int32 GPUSkinnedBones = 0;
	int64 GPUUploadBytes = 0;
};

class FSkinningStats
{
public:
	static void AddCPUMesh(int32 VertexCount, int64 UploadBytes)
	{
		Pending.CPUSkinnedMeshes++;
		Pending.CPUSkinnedVertices += VertexCount;
		Pending.CPUUploadBytes += UploadBytes;
	}

	static void AddGPUMesh(int32 BoneCount, int64 UploadBytes)
	{
		Pending.GPUSkinnedMeshes++;
		Pending.GPUSkinnedBones += BoneCount;
		Pending.GPUUploadBytes += UploadBytes;
	}

	static void EndFrame()
	{
		Last = Pending;
		Pending = FSkinningFrameStats{};
	}

	static const FSkinningFrameStats& Get() { return Last; }

	static void SetSceneGPUMs(float Ms) { SceneGPUMs = Ms; }
	static float GetSceneGPUMs() { return SceneGPUMs; }
private:
	inline static FSkinningFrameStats Pending;
	inline static FSkinningFrameStats Last;
	inline static float SceneGPUMs = 0.0f;
};

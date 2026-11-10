#pragma once
#include "Editor/Preview/PreviewEditorTab.h"
#include "Editor/Gizmo/Gizmo.h"
#include "Render/SkeletalMesh.h"

#include <filesystem>

class ASkeletalMeshActor;
class AActor;
class FPreviewViewportPanel;
class FSkeletonTreePanel;

class FViewportsPanel;

class FSkeletalMeshPreviewTab : public FPreviewEditorTab
{
public:
	explicit FSkeletalMeshPreviewTab(USkeletalMesh* InSkeletalMesh);
	virtual ~FSkeletalMeshPreviewTab() override;
	FSkeletalMeshPreviewTab(const FSkeletalMeshPreviewTab&) = delete;
	FSkeletalMeshPreviewTab& operator=(const FSkeletalMeshPreviewTab&) = delete;

	//Input을 탭 안으로 전달
	void RouteInput(const FViewportInput& BaseInput) override;

	//Bone이 선택되면 Gizmo가 따라가도록 BoneGizmoActor를 본 위치로 옮긴다. 드래그 중에는 건드리지 않는다.
	// -1이면 선택을 해제한다
	void OnBoneSelected(int32 BoneIndex);
	void UpdateBoneGizmoTarget();
	void UpdateBoneGizmo(bool bInputEnabled);
	virtual const UObject* GetEditingAsset() const { return SkeletalMesh; };
	void UpdateBoneWeightVisualization();
private:
	void DrawPreviewToolbar() override;
	void TickPreview(float DeltaTime) override;
	const FGizmo* GetPreviewGizmo() const override { return &BoneGizmo; }
	void SelectPosePath();
	void SavePose();

	// 메시 바운드가 화면에 들어오도록 Preview 카메라를 놓는다
	void FrameMesh();
	bool bPoseDirty = false;
	bool bPoseSaveFailed = false;

	std::filesystem::path PoseSavePath;

	const FGizmo* Gizmo = nullptr;
	USkeletalMesh* SkeletalMesh = nullptr;
	ASkeletalMeshActor* MeshActor = nullptr;
	FSkeletonTreePanel* SkeletonTreePanel = nullptr;
	// 본은 컴포넌트가 아니라서, Gizmo가 따라갈 빈 액터를 본 위치에 둔다. 월드 소유
	AActor* BoneGizmoActor = nullptr;
	FGizmo BoneGizmo;
	// 선택 안 함은 -1이라 부호 있는 정수여야 한다
	int32 SelectedBone = -1;
	bool bShowBoneWeights = false;
};

#pragma once

#include "Editor/Preview/PreviewEditorTab.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimNodes/AnimNode_BlendSpacePlayer.h"

class ASkeletalMeshActor;
class USkeletalMesh;
class UBlendSpace1D;

class FBlendSpaceEditor : public IEditorPanel
{
public:
	explicit FBlendSpaceEditor(FString InPath);
	~FBlendSpaceEditor() override;
	bool Init() override;
	void OnRender() override;
	void Tick(float DeltaTime) override;
	const char* GetPanelName() const override { return PanelName.c_str(); }
	USkeletalMesh* GetPreviewMesh() const { return PreviewMesh; }
	bool Evaluate(FPoseContext& Pose);
	bool ConfirmClose();
	bool WantsClose() const { return bCloseApproved; }

private:
	void RefreshAssets();
	void DrawAssets();
	void DrawSamples();
	void DrawPlayback();
	void DrawCloseConfirmation();
	void ApplySamples();
	bool Validate(FString& Error) const;
	bool Save();

	FString Path, PanelName, Status;
	UBlendSpace1D* BlendSpace = nullptr;
	USkeletalMesh* PreviewMesh = nullptr;
	TArray<FBlendSample> Samples;
	TArray<USkeleton*> Skeletons;
	TArray<USkeletalMesh*> Meshes;
	TArray<UAnimSequence*> Sequences;
	FAnimNode_BlendSpacePlayer Player;
	float X = 0.f;
	bool bLoaded = false, bDirty = false, bPlaying = false;
	bool bCloseRequested = false, bCloseApproved = false;
};

class FBlendSpaceEditorTab : public FPreviewEditorTab
{
public:
	explicit FBlendSpaceEditorTab(const FString& InPath);
	FString GetEditingFilePath() const override { return Path; }
	bool ConfirmClose() override { return Panel->ConfirmClose(); }

private:
	void BuildDefaultLayout(ImGuiID Root) override;
	void TickPreview(float DeltaTime) override;
	FString Path;
	FBlendSpaceEditor* Panel = nullptr;
	ASkeletalMeshActor* PreviewActor = nullptr;
	USkeletalMesh* CurrentMesh = nullptr;
};

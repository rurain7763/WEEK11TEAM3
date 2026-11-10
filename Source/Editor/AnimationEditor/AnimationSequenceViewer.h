#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include "Core/EngineString.h"
#include "Containers/Array.h"

class UAnimSequence;
class USkeleton;
class USkeletalMesh;

class FAnimationSequenceViewer : public IEditorPanel
{
public:
	explicit FAnimationSequenceViewer(FString InPanelName = "Animation Sequence");

	bool Init() override;
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override { return PanelName.c_str(); }

	void SetSequence(UAnimSequence* InSequence);
	UAnimSequence* GetSequence() const { return Sequence; }
	USkeleton* GetSkeleton() const { return Skeleton; }
	USkeletalMesh* GetPreviewMesh() const { return PreviewMesh; }
	float GetPreviewTime() const { return PreviewTime; }
	void RefreshAssets();

private:
	void DrawSequenceHeader();
	void DrawSequenceList();
	void SelectSkeleton(USkeleton* InSkeleton);
	void DrawPlaybackControls();
	void DrawTimeline();

private:
	FString PanelName;
	USkeleton* Skeleton = nullptr;
	USkeletalMesh* PreviewMesh = nullptr;
	UAnimSequence* Sequence = nullptr; // Non-owning asset reference.
	TArray<USkeleton*> Skeletons;
	TArray<USkeletalMesh*> Meshes;
	TArray<UAnimSequence*> Sequences;
	ImGuiTextFilter SequenceFilter;
	float PreviewTime = 0.f;
	bool bLooping = true;
	bool bPlaying = false;
	bool bRefreshAssetsRequested = false;
};

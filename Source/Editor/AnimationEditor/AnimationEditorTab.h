#pragma once

#include "Editor/Preview/PreviewEditorTab.h"
#include "AnimationSequenceViewer.h"

class ASkeletalMeshActor;
class USkeletalMesh;

// Independent editor shell using the shared preview world and viewport.
class FAnimationEditorTab : public FPreviewEditorTab
{
public:
	FAnimationEditorTab();

	FAnimationSequenceViewer* GetSequenceViewer() const { return SequenceViewer; }

private:
	void BuildDefaultLayout(ImGuiID DockSpaceRoot) override;
	void TickPreview(float DeltaTime) override;
	void DrawPreviewToolbar() override;
	void FramePreviewMesh();
	FAnimationSequenceViewer* SequenceViewer = nullptr; // Owned by Panels.
	ASkeletalMeshActor* PreviewActor = nullptr; // Owned by preview world.
	USkeletalMesh* CurrentMesh = nullptr;
};

#pragma once

#include "Editor/FEditorTab.h"
#include "PreviewScene.h"

class FPreviewViewportPanel;
struct FBox;

// Shared preview infrastructure; asset-specific actors and editing tools live in derived tabs.
class FPreviewEditorTab : public FEditorTab
{
public:
	FPreviewEditorTab(FEditorTabSettings InSettings, FString InViewportName);
	~FPreviewEditorTab() override;

	void Tick(float DeltaTime) override;
	void RouteInput(const FViewportInput& BaseInput) override;
	void CollectViews(TArray<FEditorTabView>& OutViews) override;
	UWorld* GetWorld() const override;

protected:
	// Applied once the viewport has a usable size; accepts world-space bounds.
	void FrameBounds(const FBox& Bounds);
	virtual void DrawPreviewToolbar() {}
	virtual void TickPreview(float DeltaTime) {}
	virtual const FGizmo* GetPreviewGizmo() const { return nullptr; }

	FPreviewViewportPanel* ViewportPanel = nullptr; // Owned by Panels.

private:
	void ApplyPendingCameraFrame();
	TUniquePtr<FPreviewScene> PreviewScene;
	FVector FrameCenter = FVector::ZeroVector;
	float FrameRadius = 2.f;
	bool bNeedsFraming = true;
};

#include "EnginePCH.h"
#include "PreviewEditorTab.h"
#include "Editor/Preview/PreviewViewportPanel.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Editor/Viewports/ViewportCameraMath.h"
#include "Camera/CameraTypes.h"
#include "Engine/World.h"
#include "Math/Box.h"

#include <algorithm>
#include <cmath>

FPreviewEditorTab::FPreviewEditorTab(FEditorTabSettings InSettings, FString InViewportName)
	: FEditorTab(std::move(InSettings))
	, PreviewScene(MakeUnique<FPreviewScene>())
{
	ViewportPanel = AddPanel<FPreviewViewportPanel>(GetWorld(), std::move(InViewportName), [this]()
	{
		DrawPreviewToolbar();
	});
}

FPreviewEditorTab::~FPreviewEditorTab()
{
	// Panels/viewport clients reference the world: destroy them before PreviewScene.
	Panels.Empty();
	ViewportPanel = nullptr;
}

UWorld* FPreviewEditorTab::GetWorld() const
{
	return PreviewScene->GetWorld();
}

void FPreviewEditorTab::Tick(float DeltaTime)
{
	FEditorTab::Tick(DeltaTime);
	if (bNeedsFraming && ViewportPanel->GetViewport()->IsVisible()) ApplyPendingCameraFrame();
	TickPreview(DeltaTime);
	// No game-world Tick/BeginPlay: derived tabs explicitly update their preview content.
	GetWorld()->GetScene().UpdateAllTransforms();
}

void FPreviewEditorTab::RouteInput(const FViewportInput& BaseInput)
{
	ViewportPanel->RouteInput(BaseInput);
}

void FPreviewEditorTab::CollectViews(TArray<FEditorTabView>& OutViews)
{
	FEditorTabView View;
	View.Viewport = ViewportPanel->GetViewport();
	View.OverlayClient = ViewportPanel->GetEditorClient();
	View.Gizmo = GetPreviewGizmo();
	OutViews.Add(View);
}

void FPreviewEditorTab::FrameBounds(const FBox& Bounds)
{
	FrameCenter = (Bounds.Min + Bounds.Max) * 0.5f;
	const FVector HalfExtent = (Bounds.Max - Bounds.Min) * 0.5f;
	FrameRadius = std::max(0.1f, std::sqrt(HalfExtent.SizeSquared()));
	bNeedsFraming = true;
}

void FPreviewEditorTab::ApplyPendingCameraFrame()
{
	const FMinimalViewInfo DefaultView;
	FViewCamera Camera{};
	Camera.Projection = {
		.Mode = EProjectionMode::Perspective,
		.FovDegrees = DefaultView.FOV,
		.OrthoWidth = DefaultView.OrthoWidth,
		.NearClip = DefaultView.NearClip,
		.FarClip = std::max(DefaultView.FarClip, FrameRadius * 20.f),
	};
	Camera.Transform.Rotation = ViewportCameraMath::MakeCameraRotation(0.f, -20.f);
	FEditorViewportClient* Client = ViewportPanel->GetEditorClient();
	Client->InitializeCamera(Camera, EMultipleViewportsCameraPreset::Perspective);
	Client->FocusOn(FrameCenter, FrameRadius, ViewportPanel->GetViewport()->GetOutputRect());
	bNeedsFraming = false;
}

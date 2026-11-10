#include "EnginePCH.h"
#include "AnimationEditorTab.h"
#include "Engine/World.h"
#include "GameFramework/Actor/SkeletalMeshActor.h"
#include "Animation/AnimSequence.h"
#include "Editor/Preview/PreviewViewportPanel.h"

FAnimationEditorTab::FAnimationEditorTab()
	: FPreviewEditorTab({ "Animation Editor", "AnimationEditorPreviewDock", true, true }, "Viewport##AnimationEditor")
{
	SequenceViewer = AddPanel<FAnimationSequenceViewer>("Sequence & Notifies##AnimationEditor");
}

void FAnimationEditorTab::BuildDefaultLayout(ImGuiID DockSpaceRoot)
{
	ImGuiID ViewNode = DockSpaceRoot;
	const ImGuiID ControlsNode = ImGui::DockBuilderSplitNode(ViewNode, ImGuiDir_Right, 0.32f, nullptr, &ViewNode);
	ImGui::DockBuilderDockWindow(ViewportPanel->GetPanelName(), ViewNode);
	ImGui::DockBuilderDockWindow(SequenceViewer->GetPanelName(), ControlsNode);
}

void FAnimationEditorTab::TickPreview(float DeltaTime)
{
	(void)DeltaTime;
	USkeletalMesh* SelectedMesh = SequenceViewer->GetPreviewMesh();
	if (!SelectedMesh)
	{
		if (PreviewActor) PreviewActor->GetSkeletalMeshComponent()->SetVisible(false);
		return;
	}
	if (!PreviewActor)
		PreviewActor = Cast<ASkeletalMeshActor>(GetWorld()->SpawnActor(ASkeletalMeshActor::StaticClass()));
	USkeletalMeshComponent* Component = PreviewActor->GetSkeletalMeshComponent();
	Component->SetVisible(true);
	if (CurrentMesh != SelectedMesh)
	{
		CurrentMesh = SelectedMesh;
		Component->SetSkeletalMesh(CurrentMesh);
		FramePreviewMesh();
	}

	if (UAnimSequence* Sequence = SequenceViewer->GetSequence())
	{
		// Reuse existing sampling and pose application. No game-world Tick or Notify dispatch.
		FAnimExtractContext Context;
		Context.CurrentTime = SequenceViewer->GetPreviewTime();
		FPoseContext Pose;
		Sequence->GetAnimationPose(Context, Pose);
		Component->UpdatePose(Pose);
	}
	else
	{
		Component->ResetPose();
	}
	Component->UpdateBoneMatrices();
}

void FAnimationEditorTab::DrawPreviewToolbar()
{
	ImGui::BeginDisabled(!CurrentMesh || !SequenceViewer || !SequenceViewer->GetPreviewMesh());
	if (ImGui::Button("Frame Mesh")) FramePreviewMesh();
	ImGui::EndDisabled();
}

void FAnimationEditorTab::FramePreviewMesh()
{
	if (!CurrentMesh || !PreviewActor) return;
	FrameBounds(CurrentMesh->GetMeshData().AABB.GetWorldAABB(PreviewActor->GetSkeletalMeshComponent()->GetWorldMatrix()));
}

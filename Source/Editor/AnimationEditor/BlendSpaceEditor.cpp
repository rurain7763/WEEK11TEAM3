#include "EnginePCH.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "BlendSpaceEditor.h"
#include "Core/PathUtils.h"
#include "Serialization/JsonArchive.h"
#include "UObject/UObjectIterator.h"
#include "Render/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor/SkeletalMeshActor.h"
#include "Editor/Preview/PreviewViewportPanel.h"
#include <algorithm>
#include <cmath>

namespace
{
	FString BlendAssetLabel(const UAsset* Asset)
	{
		if (!Asset) return "None";
		const auto& Name = Asset->GetPath();
		return Name.substr(Name.find_last_of("/\\") + 1);
	}
	FString BlendSequenceLabel(const UAnimSequence* Sequence)
	{
		FString Name = BlendAssetLabel(Sequence);
		const auto Separator = Name.find_last_of('|');
		return Separator == FString::npos ? Name : Name.substr(Separator + 1);
	}
}

FBlendSpaceEditor::FBlendSpaceEditor(FString InPath)
	: Path(std::move(InPath)), PanelName("BlendSpace1D Properties##" + Path) {}

FBlendSpaceEditor::~FBlendSpaceEditor() { delete BlendSpace; }

bool FBlendSpaceEditor::Init()
{
	BlendSpace = NewObject<UBlendSpace1D>();
	bLoaded = FJsonArchive::LoadBlend1DGraph(BlendSpace, Path);
	if (!bLoaded) { Status = "Could not load BlendSpace1D."; return false; }
	BlendSpace->SetAssetFilePath(Path);
	Samples = BlendSpace->GetBlendSamples();
	BlendSpace->SortBlendSamples();
	Player.SetBlendSpace(BlendSpace);
	RefreshAssets();
	return true;
}

void FBlendSpaceEditor::RefreshAssets()
{
	Skeletons.Empty(); Meshes.Empty(); Sequences.Empty();
	for (TObjectIterator<USkeleton> It; It; ++It)
		if (!It->GetPath().empty()) Skeletons.Add(*It);
	for (TObjectIterator<USkeletalMesh> It; It; ++It)
		if (!It->GetPath().empty()) Meshes.Add(*It);
	for (TObjectIterator<UAnimSequence> It; It; ++It)
		if (!It->GetPath().empty() && It->GetDataModel()) Sequences.Add(*It);
	const auto ByPath = [](const auto* A, const auto* B) { return A->GetPath() < B->GetPath(); };
	std::sort(Skeletons.begin(), Skeletons.end(), ByPath);
	std::sort(Meshes.begin(), Meshes.end(), ByPath);
	std::sort(Sequences.begin(), Sequences.end(), ByPath);
}

bool FBlendSpaceEditor::Validate(FString& Error) const
{
	if (!BlendSpace->GetSkeleton()) { Error = "Select a Skeleton."; return false; }
	if (Samples.IsEmpty()) { Error = "Add at least one sample."; return false; }
	for (int32 i = 0; i < Samples.Num(); ++i)
	{
		const auto& Sample = Samples[i];
		if (!Sample.Animation || !Sample.Animation->GetDataModel() || Sample.Animation->GetDuration() <= 0.f)
		{ Error = "Each sample needs a valid, non-empty Sequence."; return false; }
		if (Sample.Animation->GetSkeleton() != BlendSpace->GetSkeleton())
		{ Error = "All samples must use the selected Skeleton."; return false; }
		if (!std::isfinite(Sample.SampleValue.X)) { Error = "Sample X must be finite."; return false; }
		for (int32 j = 0; j < i; ++j)
			if (Sample.SampleValue.X == Samples[j].SampleValue.X)
			{ Error = "Sample X positions must be different."; return false; }
	}
	return true;
}

void FBlendSpaceEditor::ApplySamples()
{
	BlendSpace->SetBlendSamples(Samples);
	BlendSpace->SortBlendSamples();
	bDirty = true;
	Status.clear();
}

void FBlendSpaceEditor::Tick(float DeltaTime)
{
	if (!bLoaded) return;
	FString Error;
	if (!Validate(Error)) return;
	Player.SetX(X);
	FAnimationUpdateContext Context{};
	Context.RootMotion = FTransform::Identity;
	Context.DeltaTime = bPlaying && PreviewMesh ? std::max(0.f, DeltaTime) : 0.f;
	Player.Update(Context);
}

bool FBlendSpaceEditor::Evaluate(FPoseContext& Pose)
{
	FString Error;
	if (!bLoaded || !Validate(Error)) return false;
	Player.Evaluate(Pose);
	return Pose.BoneCount > 0;
}

void FBlendSpaceEditor::DrawAssets()
{
	USkeleton* Skeleton = BlendSpace->GetSkeleton();
	if (ImGui::BeginCombo("Skeleton", BlendAssetLabel(Skeleton).c_str()))
	{
		if (ImGui::IsWindowAppearing()) RefreshAssets();
		for (USkeleton* Item : Skeletons)
			if (ImGui::Selectable(Item->GetPath().c_str(), Item == Skeleton))
			{
				BlendSpace->SetSkeleton(Item);
				PreviewMesh = nullptr;
				for (auto& Sample : Samples)
					if (Sample.Animation && Sample.Animation->GetSkeleton() != Item) Sample.Animation = nullptr;
				ApplySamples(); bPlaying = false;
			}
		ImGui::EndCombo();
	}
	if (ImGui::BeginCombo("Preview Mesh", BlendAssetLabel(PreviewMesh).c_str()))
	{
		if (ImGui::IsWindowAppearing()) RefreshAssets();
		if (ImGui::Selectable("None", !PreviewMesh)) PreviewMesh = nullptr;
		for (USkeletalMesh* Item : Meshes)
			if (Skeleton && Item->GetSkeleton() == Skeleton && ImGui::Selectable(Item->GetPath().c_str(), Item == PreviewMesh)) PreviewMesh = Item;
		ImGui::EndCombo();
	}
}

void FBlendSpaceEditor::DrawSamples()
{
	FBlendSampleData Weighted[MaxBlendSampleCount]{};
	int32 Count = 0;
	int32 FirstWeightedIndex = -1;
	FString Error;
	if (Validate(Error))
	{
		BlendSpace->GetSamples(FVector(X, 0.f, 0.f), Weighted, Count);
		const auto& Sorted = BlendSpace->GetBlendSamples();
		if (Count == 1) FirstWeightedIndex = X <= Sorted[0].SampleValue.X ? 0 : Sorted.Num() - 1;
		else if (Count == 2)
			for (int32 i = 0; i + 1 < Sorted.Num(); ++i)
				if (X >= Sorted[i].SampleValue.X && X <= Sorted[i + 1].SampleValue.X) { FirstWeightedIndex = i; break; }
	}
	bool bChanged = false;
	int32 Remove = -1;
	if (ImGui::BeginTable("Samples", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Sequence", ImGuiTableColumnFlags_WidthStretch);
		const float Padding = ImGui::GetStyle().FramePadding.x * 2.f;
		ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("-1000.00").x + Padding);
		ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Weight").x + Padding);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("X").x + Padding);
		ImGui::TableHeadersRow();
		for (int32 i = 0; i < Samples.Num(); ++i)
		{
			auto& Sample = Samples[i];
			ImGui::PushID(i); ImGui::TableNextRow(); ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1.f);
			if (ImGui::BeginCombo("##Sequence", BlendSequenceLabel(Sample.Animation).c_str()))
			{
				if (ImGui::IsWindowAppearing()) RefreshAssets();
				for (UAnimSequence* Item : Sequences)
				{
					if (Item->GetSkeleton() != BlendSpace->GetSkeleton()) continue;
					ImGui::PushID(Item);
					if (ImGui::Selectable(BlendSequenceLabel(Item).c_str(), Item == Sample.Animation))
					{ Sample.Animation = Item; bChanged = true; }
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Item->GetPath().c_str());
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			if (Sample.Animation && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Sample.Animation->GetPath().c_str());
			ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1.f);
			bChanged |= ImGui::DragFloat("##SamplePosition", &Sample.SampleValue.X, 1.f, 0.f, 0.f, "%.2f");
			float Weight = 0.f;
			// Match X as well as Sequence: one clip may occupy multiple sample positions.
			if (FirstWeightedIndex >= 0)
				for (int32 j = 0; j < Count; ++j)
					if (BlendSpace->GetBlendSamples()[FirstWeightedIndex + j].SampleValue.X == Sample.SampleValue.X)
						Weight = Weighted[j].Weight;
			ImGui::TableNextColumn(); ImGui::Text("%.3f", Weight);
			ImGui::TableNextColumn(); if (ImGui::SmallButton("X##RemoveSample")) Remove = i;
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	if (Remove >= 0) { Samples.RemoveAt(Remove); bChanged = true; }
	if (ImGui::Button("Add Sample"))
	{
		float Position = 0.f;
		for (const auto& Sample : Samples) Position = std::max(Position, Sample.SampleValue.X + 100.f);
		Samples.Add(FBlendSample{ nullptr, FVector(Position, 0.f, 0.f) }); bChanged = true;
	}
	if (bChanged) ApplySamples();
}

void FBlendSpaceEditor::DrawPlayback()
{
	FString Error;
	const bool bValid = Validate(Error);
	float Min = 0.f, Max = 1.f;
	if (!Samples.IsEmpty())
	{
		Min = Max = Samples[0].SampleValue.X;
		for (const auto& Sample : Samples) { Min = std::min(Min, Sample.SampleValue.X); Max = std::max(Max, Sample.SampleValue.X); }
		if (Min == Max) Max = Min + 1.f;
	}
	ImGui::SliderFloat("Preview X", &X, Min, Max, "%.2f");
	ImGui::BeginDisabled(!bValid || !PreviewMesh);
	if (ImGui::Button(bPlaying ? "Pause" : "Play")) bPlaying = !bPlaying;
	ImGui::SameLine();
	if (ImGui::Button("Stop")) { bPlaying = false; Player.Initialize(); }
	ImGui::EndDisabled();
	if (!bValid) ImGui::TextWrapped("%s", Error.c_str());
}

bool FBlendSpaceEditor::Save()
{
	if (!Validate(Status)) return false;
	ApplySamples();
	const auto Destination = PathUtils::FromUtf8(Path);
	auto Temporary = Destination;
	Temporary += L".tmp";
	for (int32 i = 1; std::filesystem::exists(Temporary); ++i) Temporary = Destination.wstring() + L".tmp" + std::to_wstring(i);
	if (!FJsonArchive::SaveBlend1D(BlendSpace, PathUtils::ToUtf8(Temporary)) ||
		!MoveFileExW(Temporary.c_str(), Destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		std::error_code Error; std::filesystem::remove(Temporary, Error);
		Status = "Could not save BlendSpace1D."; return false;
	}
	bDirty = false; Status = "Saved."; return true;
}

bool FBlendSpaceEditor::ConfirmClose()
{
	if (!bDirty || bCloseApproved) return true;
	SetOpen(true); bCloseRequested = true; return false;
}

void FBlendSpaceEditor::DrawCloseConfirmation()
{
	if (bCloseRequested) { ImGui::OpenPopup("Unsaved BlendSpace1D"); bCloseRequested = false; }
	if (ImGui::BeginPopupModal("Unsaved BlendSpace1D", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Save changes before closing?");
		if (ImGui::Button("Save and Close") && Save()) { bCloseApproved = true; ImGui::CloseCurrentPopup(); }
		ImGui::SameLine();
		if (ImGui::Button("Discard")) { bCloseApproved = true; ImGui::CloseCurrentPopup(); }
		ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		if (!Status.empty()) ImGui::TextWrapped("%s", Status.c_str());
		ImGui::EndPopup();
	}
}

void FBlendSpaceEditor::OnRender()
{
	if (ImGui::Begin(GetPanelName()))
	{
		if (ImGui::IsWindowAppearing()) RefreshAssets();
		if (bLoaded)
		{
			if (ImGui::Button("Save")) Save();
			ImGui::SameLine(); ImGui::TextDisabled("%s", bDirty ? "Unsaved changes" : "Saved");
			DrawAssets(); ImGui::Separator(); DrawSamples(); ImGui::Separator(); DrawPlayback();
		}
		if (!Status.empty()) ImGui::TextWrapped("%s", Status.c_str());
		DrawCloseConfirmation();
	}
	ImGui::End();
}

FBlendSpaceEditorTab::FBlendSpaceEditorTab(const FString& InPath)
	: FPreviewEditorTab({ "BlendSpace1D - " + PathUtils::ToUtf8(PathUtils::FromUtf8(InPath).filename()) + "###" + InPath,
		"BlendSpaceDock##" + InPath, true, true }, "Viewport##BlendSpace" + InPath), Path(InPath)
{
	Panel = AddPanel<FBlendSpaceEditor>(Path);
}

void FBlendSpaceEditorTab::BuildDefaultLayout(ImGuiID Root)
{
	ImGuiID View = Root;
	const ImGuiID Properties = ImGui::DockBuilderSplitNode(View, ImGuiDir_Right, .4f, nullptr, &View);
	ImGui::DockBuilderDockWindow(ViewportPanel->GetPanelName(), View);
	ImGui::DockBuilderDockWindow(Panel->GetPanelName(), Properties);
}

void FBlendSpaceEditorTab::TickPreview(float DeltaTime)
{
	if (Panel->WantsClose()) *GetOpenFlag() = false;
	USkeletalMesh* Mesh = Panel->GetPreviewMesh();
	if (!Mesh) { if (PreviewActor) PreviewActor->GetSkeletalMeshComponent()->SetVisible(false); return; }
	if (!PreviewActor) PreviewActor = Cast<ASkeletalMeshActor>(GetWorld()->SpawnActor(ASkeletalMeshActor::StaticClass()));
	auto* Component = PreviewActor->GetSkeletalMeshComponent();
	Component->SetVisible(true);
	if (CurrentMesh != Mesh)
	{
		CurrentMesh = Mesh; Component->SetSkeletalMesh(Mesh);
		FrameBounds(Mesh->GetMeshData().AABB.GetWorldAABB(Component->GetWorldMatrix()));
	}
	FPoseContext Pose{};
	if (Panel->Evaluate(Pose)) Component->UpdatePose(Pose);
	else Component->ResetPose();
	Component->UpdateBoneMatrices();
}

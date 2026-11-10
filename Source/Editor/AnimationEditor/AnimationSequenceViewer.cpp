#include "EnginePCH.h"
#include "AnimationSequenceViewer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimDataModel.h"
#include "Animation/Skeleton.h"
#include "Render/SkeletalMesh.h"
#include "UObject/UObjectIterator.h"
#include "Serialization/JsonArchive.h"
#include <imgui_neo_sequencer.h>

#include <algorithm>
#include <cmath>

namespace
{
	FString AssetLabel(const UAsset* Asset)
	{
		if (!Asset) return "None";
		const FString& Path = Asset->GetPath();
		return Path.substr(Path.find_last_of("/\\") + 1);
	}
}

FAnimationSequenceViewer::FAnimationSequenceViewer(FString InPanelName)
	: PanelName(std::move(InPanelName))
{
}

bool FAnimationSequenceViewer::Init()
{
	RefreshAssets();
	return true;
}

void FAnimationSequenceViewer::RefreshAssets()
{
	Skeletons.Empty();
	Meshes.Empty();
	Sequences.Empty();
	// Registered assets have paths; omit transient test clips and default objects.
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

void FAnimationSequenceViewer::SelectSkeleton(USkeleton* InSkeleton)
{
	if (Skeleton == InSkeleton) return;
	Skeleton = InSkeleton;
	PreviewMesh = nullptr;
	SetSequence(nullptr);
	SequenceFilter.Clear();
	bRefreshAssetsRequested = true;
}

void FAnimationSequenceViewer::SetSequence(UAnimSequence* InSequence)
{
	if (InSequence && InSequence->GetSkeleton() != Skeleton) return;
	Sequence = InSequence;
	SelectedNotify = -1;
	SaveStatus.clear();
	if (Sequence) Sequence->SortNotifies();
	PreviewTime = 0.f;
	bPlaying = false;
}

void FAnimationSequenceViewer::Tick(float DeltaTime)
{
	if (bRefreshAssetsRequested)
	{
		RefreshAssets();
		bRefreshAssetsRequested = false;
	}
	if (!bPlaying || !Sequence || !PreviewMesh) return;
	const float Duration = Sequence->GetDuration();
	if (Duration <= 0.f) { bPlaying = false; return; }
	const float Step = std::max(0.f, DeltaTime);
	PreviewTime += bReversePlay ? -Step : Step;
	if (bReversePlay && PreviewTime <= 0.f)
	{
		if (bLooping)
		{
			const float Overshoot = -PreviewTime;
			PreviewTime = Duration - std::fmod(Overshoot, Duration);
		}
		else
		{
			PreviewTime = 0.f;
			bPlaying = false;
		}
	}
	else if (!bReversePlay && PreviewTime >= Duration)
	{
		if (bLooping)
		{
			PreviewTime = std::fmod(PreviewTime, Duration);
		}
		else
		{
			PreviewTime = Duration;
			bPlaying = false;
		}
	}
}

void FAnimationSequenceViewer::OnRender()
{
	bool bOpen = IsOpen();
	if (ImGui::Begin(GetPanelName(), &bOpen))
	{
		if (ImGui::IsWindowAppearing()) RefreshAssets();
		DrawSequenceHeader();
		ImGui::Separator();
		DrawSequenceList();
		DrawPlaybackControls();
		ImGui::Separator();
		DrawTimeline();
	}
	ImGui::End();
	SetOpen(bOpen);
}

void FAnimationSequenceViewer::DrawSequenceHeader()
{
	ImGui::TextUnformatted("Preview Assets");
	ImGui::TextUnformatted("Skeleton");
	ImGui::SetNextItemWidth(-1.f);
	if (ImGui::BeginCombo("##Skeleton", AssetLabel(Skeleton).c_str()))
	{
		if (ImGui::IsWindowAppearing()) RefreshAssets();
		if (ImGui::Selectable("None", Skeleton == nullptr)) SelectSkeleton(nullptr);
		for (USkeleton* Item : Skeletons)
		{
			ImGui::PushID(Item);
			if (ImGui::Selectable(AssetLabel(Item).c_str(), Skeleton == Item)) SelectSkeleton(Item);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Item->GetPath().c_str());
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

	ImGui::BeginDisabled(!Skeleton);
	ImGui::TextUnformatted("Preview Mesh");
	ImGui::SetNextItemWidth(-1.f);
	if (ImGui::BeginCombo("##PreviewMesh", AssetLabel(PreviewMesh).c_str()))
	{
		if (ImGui::IsWindowAppearing()) RefreshAssets();
		if (ImGui::Selectable("None", PreviewMesh == nullptr)) { PreviewMesh = nullptr; bPlaying = false; }
		bool bAnyMesh = false;
		for (USkeletalMesh* Item : Meshes)
		{
			if (Item->GetSkeleton() != Skeleton) continue;
			bAnyMesh = true;
			ImGui::PushID(Item);
			if (ImGui::Selectable(AssetLabel(Item).c_str(), PreviewMesh == Item)) PreviewMesh = Item;
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Item->GetPath().c_str());
			ImGui::PopID();
		}
		if (!bAnyMesh) ImGui::TextDisabled("No compatible meshes");
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
}

void FAnimationSequenceViewer::DrawSequenceList()
{
	ImGui::TextUnformatted("Animation Sequences");
	ImGui::BeginDisabled(!Skeleton);
	ImGui::TextUnformatted("Filter");
	ImGui::SameLine();
	SequenceFilter.Draw("##ClipFilter", std::min(240.f, ImGui::GetContentRegionAvail().x));
	if (ImGui::BeginChild("SequenceList", ImVec2(0.f, 155.f), ImGuiChildFlags_Borders))
	{
		int Count = 0;
		if (Skeleton)
		{
			for (UAnimSequence* Item : Sequences)
			{
				if (Item->GetSkeleton() != Skeleton || !SequenceFilter.PassFilter(Item->GetPath().c_str())) continue;
				++Count;
				ImGui::PushID(Item);
				if (ImGui::Selectable(AssetLabel(Item).c_str(), Sequence == Item)) SetSequence(Item);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Item->GetPath().c_str());
				ImGui::PopID();
			}
		}
		if (Count == 0) ImGui::TextDisabled(Skeleton ? "No matching sequences" : "Select a Skeleton");
	}
	ImGui::EndChild();
	ImGui::EndDisabled();
}

void FAnimationSequenceViewer::DrawPlaybackControls()
{
	ImGui::TextUnformatted("Playback");
	const float Duration = Sequence ? Sequence->GetDuration() : 0.f;
	ImGui::BeginDisabled(!Sequence || !PreviewMesh || Duration <= 0.f);
	if (ImGui::Button(bPlaying ? "Pause" : "Play"))
	{
		if (!bPlaying)
		{
			if (bReversePlay && PreviewTime <= 0.f)
				PreviewTime = Duration;
			else if (!bReversePlay && PreviewTime >= Duration)
				PreviewTime = 0.f;
		}
		bPlaying = !bPlaying;
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) { bPlaying = false; PreviewTime = 0.f; }
	ImGui::SameLine();
	ImGui::Checkbox("Loop", &bLooping);
	ImGui::Checkbox("Reverse", &bReversePlay);
	ImGui::EndDisabled();
}

void FAnimationSequenceViewer::DrawTimeline()
{
	ImGui::TextUnformatted("Timeline");
	if (!Sequence || !Sequence->GetDataModel())
	{
		ImGui::TextDisabled("Select an animation sequence to view its timeline.");
		return;
	}
	const int32 SampleRate = Sequence->GetDataModel()->GetSampleRate();
	const float Duration = Sequence->GetDuration();
	if (SampleRate <= 0 || Duration <= 0.f)
	{
		ImGui::TextDisabled("This sequence has no valid sample rate or duration.");
		return;
	}

	// The final sample is inclusive: a 1-second, 30-Hz clip spans frames 0..30.
	int32_t StartFrame = 0;
	int32_t EndFrame = std::max(1, static_cast<int32_t>(std::lround(Duration * SampleRate)));
	int32_t CurrentFrame = std::clamp(static_cast<int32_t>(std::lround(PreviewTime * SampleRate)), StartFrame, EndFrame);
	const int32_t FrameBeforeInput = CurrentFrame;
	ImGui::TextDisabled("Frame %d / %d   |   %d fps", CurrentFrame, EndFrame, SampleRate);

	// Keep each clip's timeline view separate; changing clips starts at frame zero.
	ImGui::PushID(Sequence);
	if (ImGui::BeginNeoSequencer("AnimationTimeline", &CurrentFrame, &StartFrame, &EndFrame,
		ImVec2(0.f, 170.f), ImGuiNeoSequencerFlags_HideZoom))
	{
		// Boundary markers show the clip extent, not editable animation keys.
		if (ImGui::BeginNeoTimelineEx("Animation", nullptr, ImGuiNeoTimelineFlags_None))
		{
			int32_t FirstSample = StartFrame;
			int32_t LastSample = EndFrame;
			ImGui::NeoKeyframe(&FirstSample);
			if (ImGui::IsNeoKeyframeHovered()) ImGui::SetTooltip("Clip start: 0 s");
			ImGui::NeoKeyframe(&LastSample);
			if (ImGui::IsNeoKeyframeHovered()) ImGui::SetTooltip("Clip end: %.3f s", Duration);
			ImGui::EndNeoTimeLine();
		}
		// Display markers only; editing event times requires a separate sequence API.
		if (ImGui::BeginNeoTimelineEx("Notifies", nullptr, ImGuiNeoTimelineFlags_None))
		{
			const auto& Notifies = Sequence->GetNotifies();
			for (int32 Index = 0; Index < Notifies.Num(); ++Index)
			{
				const auto& Notify = Notifies[Index];
				if (!std::isfinite(Notify.TriggerTime) || Notify.TriggerTime < 0.f || Notify.TriggerTime > Duration) continue;
				ImGui::PushID(Index);
				int32_t Frame = static_cast<int32_t>(std::lround(Notify.TriggerTime * SampleRate));
				ImGui::NeoKeyframe(&Frame);
				if (ImGui::IsNeoKeyframeHovered())
				{
					ImGui::SetTooltip("%s: %.3f s", Notify.Name.ToString().c_str(), Notify.TriggerTime);
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) SelectedNotify = Index;
				}
				ImGui::PopID();
			}
			ImGui::EndNeoTimeLine();
		}
		ImGui::EndNeoSequencer();
	}
	ImGui::PopID();

	// Preserve continuous playback time unless the user actually moves the frame cursor.
	if (CurrentFrame != FrameBeforeInput)
	{
		CurrentFrame = std::clamp(CurrentFrame, StartFrame, EndFrame);
		PreviewTime = std::min(Duration, static_cast<float>(CurrentFrame) / SampleRate);
		bPlaying = false;
	}
	ImGui::Text("Time: %.3f / %.3f s", PreviewTime, Duration);

	ImGui::InputText("Notify Name", NotifyName, sizeof(NotifyName));
	ImGui::Text("Notify Time: %.3f s (current cursor)", PreviewTime);
	const FName NewNotifyName(NotifyName);
	ImGui::BeginDisabled(NewNotifyName.IsNone() || !std::isfinite(PreviewTime) || PreviewTime < 0.f || PreviewTime > Duration);
	if (ImGui::Button("Add Notify"))
	{
		Sequence->AddNotify(FAnimNotifyEvent{ NewNotifyName, PreviewTime });
		Sequence->SortNotifies();
		SelectedNotify = -1;
		ModifiedSequences.insert(Sequence);
		SaveStatus.clear();
	}
	ImGui::EndDisabled();

	const auto& Notifies = Sequence->GetNotifies();
	if (!Notifies.IsValidIndex(SelectedNotify)) SelectedNotify = -1;
	if (ImGui::BeginChild("NotifyList", ImVec2(0.f, 110.f), ImGuiChildFlags_Borders))
	{
		if (Notifies.IsEmpty()) ImGui::TextDisabled("No notifies");
		for (int32 Index = 0; Index < Notifies.Num(); ++Index)
		{
			ImGui::PushID(Index);
			const auto& Notify = Notifies[Index];
			const FString Label = Notify.Name.ToString() + " @ " + std::to_string(Notify.TriggerTime) + " s";
			if (ImGui::Selectable(Label.c_str(), SelectedNotify == Index)) SelectedNotify = Index;
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
	ImGui::BeginDisabled(SelectedNotify < 0);
	if (ImGui::Button("Delete Selected Notify"))
	{
		Sequence->RemoveNotify(SelectedNotify);
		SelectedNotify = -1;
		ModifiedSequences.insert(Sequence);
		SaveStatus.clear();
	}
	ImGui::EndDisabled();
	bool bRootMotion = Sequence->HasRootMotion();
	if (ImGui::Checkbox("Enable Root Motion", &bRootMotion))
	{
		Sequence->EnableRootMotion(bRootMotion);
		ModifiedSequences.insert(Sequence);
		SaveStatus.clear();
	}
	const FString& FilePath = Sequence->GetAssetFilePath();
	ImGui::TextWrapped("File: %s", FilePath.empty() ? "No save path assigned" : FilePath.c_str());
	ImGui::BeginDisabled(FilePath.empty());
	if (ImGui::Button("Save"))
	{
		if (FJsonArchive::SaveAnimSequence(Sequence, FilePath))
		{
			ModifiedSequences.erase(Sequence);
			SaveStatus = "Saved.";
		}
		else SaveStatus = "Save failed.";
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextDisabled("%s", ModifiedSequences.contains(Sequence) ? "Unsaved changes" : "No pending changes");
	if (!SaveStatus.empty()) ImGui::TextWrapped("%s", SaveStatus.c_str());
}

#include "EnginePCH.h"
#include "AnimationSequenceViewer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimDataModel.h"
#include "Animation/Skeleton.h"
#include "Render/SkeletalMesh.h"
#include "UObject/UObjectIterator.h"
#include "Serialization/JsonArchive.h"
#include "Editor/Details/DetailsPanel.h"
#include "Editor/HitoriEd/EditorEngine.h"
#include "Engine/Engine.h"
#include "UObject/Class.h"
#include "UObject/UObjectHash.h"
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
	bNotifyOrderDirty = false;
	DraggedPointNotify = -1;
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

void FAnimationSequenceViewer::SortSelectedNotify()
{
	const FAnimNotifyEvent* Notify = Sequence->GetNotify(SelectedNotify);
	if (!Notify)
	{
		Sequence->SortNotifies();
		bNotifyOrderDirty = false;
		return;
	}
	const FAnimNotifyEvent Edited = *Notify;
	Sequence->SortNotifies();
	const auto& Notifies = Sequence->GetNotifies();
	for (int32 Index = 0; Index < Notifies.Num(); ++Index)
	{
		const FAnimNotifyEvent& Item = Notifies[Index];
		if (Item.Name == Edited.Name && Item.TriggerTime == Edited.TriggerTime
			&& Item.Duration == Edited.Duration && Item.NotifyState == Edited.NotifyState)
		{
			SelectedNotify = Index;
			break;
		}
	}
	bNotifyOrderDirty = false;
}

void FAnimationSequenceViewer::AddNotifyAtCursor(UClass* StateClass)
{
	const float ClipDuration = Sequence->GetDuration();
	const float FrameTime = 1.f / Sequence->GetDataModel()->GetSampleRate();
	FAnimNotifyEvent Notify;
	Notify.TriggerTime = std::clamp(PreviewTime, 0.f, ClipDuration);
	if (StateClass)
	{
		Notify.TriggerTime = std::min(Notify.TriggerTime, std::max(0.f, ClipDuration - FrameTime));
		Notify.NotifyState = NewObject<UAnimNotifyState>(Sequence, StateClass);
		if (!Notify.NotifyState)
		{
			SaveStatus = "Could not create selected NotifyState.";
			return;
		}
		Notify.Name = FName(StateClass->Name);
		Notify.Duration = std::min(std::max(0.2f, FrameTime), ClipDuration - Notify.TriggerTime);
	}
	else
	{
		Notify.Name = FName(NotifyName);
		if (Notify.Name.IsNone()) return;
	}
	Sequence->AddNotify(Notify);
	SelectedNotify = Sequence->GetNotifies().Num() - 1;
	SortSelectedNotify();
	ModifiedSequences.insert(Sequence);
	SaveStatus.clear();
}

void FAnimationSequenceViewer::DrawAddNotifyMenu()
{
	if (ImGui::BeginMenu("New Notify"))
	{
		ImGui::InputText("Name", NotifyName, sizeof(NotifyName));
		ImGui::BeginDisabled(FName(NotifyName).IsNone());
		if (ImGui::MenuItem("Add at playhead")) AddNotifyAtCursor(nullptr);
		ImGui::EndDisabled();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("New Notify State"))
	{
		NotifyClassFilter.Draw("Search", 180.f);
		TArray<UClass*> Classes;
		GetDerivedClasses(UAnimNotifyState::StaticClass(), Classes);
		std::sort(Classes.begin(), Classes.end(), [](const UClass* A, const UClass* B) { return A->Name < B->Name; });
		bool bAny = false;
		for (UClass* Class : Classes)
		{
			if (!Class->Constructor || !NotifyClassFilter.PassFilter(Class->Name.c_str())) continue;
			bAny = true;
			if (ImGui::MenuItem(Class->Name.c_str())) AddNotifyAtCursor(Class);
		}
		if (!bAny) ImGui::TextDisabled("No matching NotifyState classes.");
		ImGui::EndMenu();
	}
}

void FAnimationSequenceViewer::DrawNotifyDetails()
{
	FAnimNotifyEvent* Notify = Sequence->GetNotify(SelectedNotify);
	if (!Notify) return;

	ImGui::PushID("SelectedNotifyDetails");
	ImGui::Separator();
	ImGui::Text("Selected Notify: %s", Notify->Name.ToString().c_str());
	const float ClipDuration = Sequence->GetDuration();
	bool bChanged = false;
	char NameBuffer[256];
	strncpy_s(NameBuffer, Notify->Name.ToString().c_str(), _TRUNCATE);
	if (ImGui::InputText("Name", NameBuffer, sizeof(NameBuffer)) && !FName(NameBuffer).IsNone())
	{
		Notify->Name = FName(NameBuffer);
		bChanged = true;
	}
	float StartTime = Notify->TriggerTime;
	const float Step = 1.f / Sequence->GetDataModel()->GetSampleRate();
	if (ImGui::DragFloat("Start (s)", &StartTime, Step, 0.f, ClipDuration, "%.3f", ImGuiSliderFlags_AlwaysClamp)
		&& std::isfinite(StartTime))
	{
		Notify->TriggerTime = std::clamp(StartTime, 0.f, ClipDuration);
		Notify->Duration = std::clamp(Notify->Duration, 0.f, ClipDuration - Notify->TriggerTime);
		bChanged = true;
		bNotifyOrderDirty = true;
	}
	float NotifyDuration = Notify->Duration;
	if (ImGui::DragFloat("Duration (s)", &NotifyDuration, Step, 0.f, ClipDuration - Notify->TriggerTime, "%.3f", ImGuiSliderFlags_AlwaysClamp)
		&& std::isfinite(NotifyDuration))
	{
		Notify->Duration = std::clamp(NotifyDuration, 0.f, ClipDuration - Notify->TriggerTime);
		bChanged = true;
	}
	ImGui::Text("End Time: %.3f s", Notify->TriggerTime + Notify->Duration);

	UClass* CurrentClass = Notify->NotifyState ? Notify->NotifyState->GetClass() : nullptr;
	if (ImGui::BeginCombo("Notify State", CurrentClass ? CurrentClass->Name.c_str() : "None"))
	{
		UClass* PickedClass = CurrentClass;
		bool bPicked = false;
		if (ImGui::Selectable("None", !CurrentClass))
		{
			PickedClass = nullptr;
			bPicked = true;
		}
		TArray<UClass*> Classes;
		GetDerivedClasses(UAnimNotifyState::StaticClass(), Classes);
		std::sort(Classes.begin(), Classes.end(), [](const UClass* A, const UClass* B) { return A->Name < B->Name; });
		int32 AvailableCount = 0;
		ImGui::BeginDisabled(Notify->TriggerTime >= ClipDuration);
		for (UClass* Class : Classes)
		{
			if (!Class->Constructor) continue;
			++AvailableCount;
			if (ImGui::Selectable(Class->Name.c_str(), Class == CurrentClass))
			{
				PickedClass = Class;
				bPicked = true;
			}
		}
		ImGui::EndDisabled();
		if (AvailableCount == 0) ImGui::TextDisabled("No registered NotifyState subclasses.");
		if (bPicked && PickedClass != CurrentClass)
		{
			UAnimNotifyState* NewState = PickedClass ? NewObject<UAnimNotifyState>(Sequence, PickedClass) : nullptr;
			if (!PickedClass || NewState)
			{
				UAnimNotifyState* OldState = Notify->NotifyState;
				Notify->NotifyState = NewState;
				if (OldState && OldState->GetOuter() == Sequence) delete OldState;
				if (!NewState) Notify->Duration = 0.f;
				else if (Notify->Duration <= 0.f)
					Notify->Duration = std::min(1.f / Sequence->GetDataModel()->GetSampleRate(), ClipDuration - Notify->TriggerTime);
				bChanged = true;
			}
			else SaveStatus = "Could not create selected NotifyState.";
		}
		ImGui::EndCombo();
	}
	if (Notify->Duration > 0.f && !Notify->NotifyState)
		ImGui::TextDisabled("Select a Notify State to handle this interval.");
	if (Notify->NotifyState)
		bChanged |= FDetailsPanel::DrawObjectProperties(Notify->NotifyState);
	if (bChanged)
	{
		ModifiedSequences.insert(Sequence);
		SaveStatus.clear();
	}
	ImGui::PopID();
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
	const UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
	const bool bEditingLocked = EditorEngine && EditorEngine->IsPlaying();
	ImGui::BeginDisabled(bEditingLocked);
	if (ImGui::Button("+ Add Notify")) ImGui::OpenPopup("AddNotifyMenu");
	if (ImGui::BeginPopup("AddNotifyMenu"))
	{
		DrawAddNotifyMenu();
		ImGui::EndPopup();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Text("at %.3f s", PreviewTime);
	int32 DeleteNotify = -1;

	// Keep each clip's timeline view separate; changing clips starts at frame zero.
	ImGui::PushID(Sequence);
	if (ImGui::BeginNeoSequencer("AnimationTimeline", &CurrentFrame, &StartFrame, &EndFrame,
		ImVec2(0.f, std::clamp(100.f + Sequence->GetNotifies().Num() * 28.f, 170.f, 360.f)), ImGuiNeoSequencerFlags_HideZoom))
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
		const auto& Notifies = Sequence->GetNotifies();
		if (Notifies.IsEmpty() && ImGui::BeginNeoTimelineEx("Notifies", nullptr, ImGuiNeoTimelineFlags_None))
		{
			ImGui::EndNeoTimeLine();
		}
		for (int32 Index = 0; Index < Notifies.Num(); ++Index)
		{
			FAnimNotifyEvent* Notify = Sequence->GetNotify(Index);
			if (!std::isfinite(Notify->TriggerTime) || Notify->TriggerTime < 0.f || Notify->TriggerTime > Duration) continue;
			ImGui::PushID(Index);
			const FString Name = Notify->Name.ToString();
			if (ImGui::BeginNeoTimelineEx(Name.c_str(), nullptr, ImGuiNeoTimelineFlags_None))
			{
				const bool bState = Notify->Duration > 0.f;
				int32_t From = std::clamp(static_cast<int32_t>(std::lround(Notify->TriggerTime * SampleRate)), 0, EndFrame);
				int32_t To = bState ? std::clamp(static_cast<int32_t>(std::lround((Notify->TriggerTime + Notify->Duration) * SampleRate)), From + 1, EndFrame + 1) : From + 1;
				const int32_t OldFrom = From;
				const int32_t OldTo = To;
				ImGui::BeginDisabled(bEditingLocked);
				bool bHovered = false;
				if (bState)
				{
					if (ImGui::NeoSection("Notify", &From, &To, Name.c_str(), SelectedNotify == Index, 0, EndFrame)) SelectedNotify = Index;
					bHovered = ImGui::IsItemHovered();
				}
				else
				{
					ImGui::NeoKeyframe(&From);
					bHovered = ImGui::IsNeoKeyframeHovered();
					if (!bEditingLocked && bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						SelectedNotify = Index;
						DraggedPointNotify = Index;
						DraggedPointStartFrame = OldFrom;
						DraggedPointMouseX = ImGui::GetIO().MousePos.x;
					}
					From = OldFrom;
					if (!bEditingLocked && DraggedPointNotify == Index && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
					{
						const float FrameDelta = (ImGui::GetIO().MousePos.x - DraggedPointMouseX) / std::max(ImGui::NeoGetFrameWidth(), 0.001f);
						From = std::clamp(DraggedPointStartFrame + static_cast<int32_t>(std::lround(FrameDelta)), 0, EndFrame);
					}
					To = From + 1;
				}
				if (bHovered)
				{
					ImGui::SetTooltip("%s\nStart: %.3f s\nDuration: %.3f s", Name.c_str(), Notify->TriggerTime, Notify->Duration);
				}
				if (!bEditingLocked && bHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) ImGui::OpenPopup("NotifyActions");
				if (ImGui::BeginPopup("NotifyActions"))
				{
					SelectedNotify = Index;
					if (ImGui::MenuItem("Delete", "Del")) DeleteNotify = Index;
					ImGui::EndPopup();
				}
				if (!bEditingLocked && (From != OldFrom || To != OldTo))
				{
					SelectedNotify = Index;
					Notify->TriggerTime = std::clamp(static_cast<float>(From) / SampleRate, 0.f, Duration);
					if (bState) Notify->Duration = std::clamp(static_cast<float>(To) / SampleRate - Notify->TriggerTime, 0.f, Duration - Notify->TriggerTime);
					ModifiedSequences.insert(Sequence);
					SaveStatus.clear();
					bNotifyOrderDirty = true;
				}
				ImGui::EndDisabled();
				ImGui::EndNeoTimeLine();
			}
			ImGui::PopID();
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

	if (bEditingLocked) ImGui::TextDisabled("Stop Play to edit notifies.");
	else ImGui::TextDisabled("Drag to move. Drag interval edges to resize. Right-click to delete.");
	ImGui::BeginDisabled(bEditingLocked);
	if (!Sequence->GetNotifies().IsValidIndex(SelectedNotify)) SelectedNotify = -1;
	DrawNotifyDetails();
	ImGui::BeginDisabled(SelectedNotify < 0);
	if (ImGui::Button("Delete Selected Notify") || (!ImGui::IsAnyItemActive() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete))) DeleteNotify = SelectedNotify;
	ImGui::EndDisabled();
	if (!bEditingLocked && Sequence->GetNotifies().IsValidIndex(DeleteNotify))
	{
		UAnimNotifyState* State = Sequence->GetNotify(DeleteNotify)->NotifyState;
		Sequence->RemoveNotify(DeleteNotify);
		if (State && State->GetOuter() == Sequence) delete State;
		SelectedNotify = -1;
		ModifiedSequences.insert(Sequence);
		SaveStatus.clear();
	}
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) DraggedPointNotify = -1;
	if (bNotifyOrderDirty && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive()) SortSelectedNotify();
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

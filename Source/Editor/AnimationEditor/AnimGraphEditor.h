#pragma once

#include "Editor/FEditorTab.h"
#include "Containers/Array.h"
#include "json.hpp"
#include "ImNodesEz.h"

class UAnimSequence;

// Edits the existing AnimGraph JSON schema; runtime graph construction stays in UAnimGraph.
class FAnimGraphEditor : public IEditorPanel
{
public:
	explicit FAnimGraphEditor(FString InPath);
	~FAnimGraphEditor() override;
	bool Init() override;
	void Tick(float DeltaTime) override {}
	void OnRender() override;
	const char* GetPanelName() const override { return PanelName.c_str(); }
	bool IsDirty() const { return bDirty; }
	bool ConfirmClose();
	bool WantsClose() const { return bCloseApproved; }

private:
	struct FStateView
	{
		int32 ID = -1;
		FString Slot;
		ImVec2 Position;
		bool bSelected = false;
	};
	struct FNode
	{
		int32 ID = -1;
		int32 Type = 0;
		int32 DataIndex = -1;
		ImVec2 Position;
		bool bSelected = false;
		TArray<TUniquePtr<FStateView>> States;
		FStateView AnyState;
		ImNodes::Ez::Context* StateContext = nullptr;
	};
	bool Load();
	bool Save();
	void DrawToolbar();
	void DrawCanvas();
	void DrawDetails();
	void DrawStateCanvas(FNode& Machine);
	bool DrawStateNode(FNode& Machine, FStateView& State);
	void AddState(FNode& Machine, const ImVec2& Position);
	void DrawStateMachineDetails(FNode& Machine);
	void DrawTransitions(FNode& Machine, const char* Field);
	void DrawParameters();
	void RefreshAssets();
	void SyncStates(FNode& Machine);
	FStateView* FindState(FNode& Machine, int32 ID) const;
	void AddTransition(FNode& Machine, int32 From, int32 To);
	void DrawStateCombo(const char* Label, FNode& Machine, nlohmann::json& Field);
	void DrawCloseConfirmation();
	void AddNode(int32 Type);
	void DeleteSelectedNode();
	void AcceptConnection(FNode* Input, const char* Slot, FNode* Output);
	bool CanReach(int32 StartID, int32 TargetID) const;
	FNode* FindNode(int32 ID) const;
	nlohmann::json& Data(FNode& Node);
	const nlohmann::json& Data(const FNode& Node) const;
	bool ValidateForSave();

	FString Path;
	FString PanelName;
	FString Status;
	nlohmann::json Document;
	TArray<TUniquePtr<FNode>> Nodes;
	TArray<UAnimSequence*> Sequences;
	TArray<FString> BlendSpacePaths;
	ImNodes::Ez::Context* Context = nullptr;
	int32 SelectedID = -1;
	int32 NextID = 1;
	int32 ViewedMachineID = -1;
	int32 SelectedStateID = -1;
	ImVec2 NewNodePosition;
	bool bLoaded = false;
	bool bDirty = false;
	bool bStatusError = false;
	bool bCloseRequested = false;
	bool bCloseApproved = false;
};

class FAnimGraphEditorTab : public FEditorTab
{
public:
	explicit FAnimGraphEditorTab(const FString& InPath);
	FString GetEditingFilePath() const override { return Path; }
	bool ConfirmClose() override { return Panel->ConfirmClose(); }
	void Tick(float DeltaTime) override;

private:
	FString Path;
	FAnimGraphEditor* Panel = nullptr;
};

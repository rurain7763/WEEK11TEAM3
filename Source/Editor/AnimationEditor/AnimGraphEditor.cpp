#include "EnginePCH.h"
#include "AnimGraphEditor.h"
#include "Animation/AnimSequence.h"
#include "UObject/UObjectIterator.h"
#include "Core/PathUtils.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <cmath>

namespace
{
	const char* NodeTitle(int32 Type)
	{
		switch (Type)
		{
			case 0: return "Sequence Player";
			case 1: return "Two Way Blend";
			case 2: return "Blend Space Player";
			case 3: return "State Machine";
			default: return "Unknown";
		}
	}
	const char* DataArrayName(int32 Type)
	{
		switch (Type)
		{
			case 0: return "AnimSequencePlayers";
			case 1: return "AnimTwoWayBlends";
			case 2: return "BlendSpacePlayers";
			default: return "StateMachines";
		}
	}
	bool EditGraphText(const char* Label, nlohmann::json& Field)
	{
		const FString Current = Field.get<FString>();
		TArray<char> Buffer;
		Buffer.SetNum(static_cast<int32>(Current.size()) + 1024);
		std::memcpy(Buffer.GetData(), Current.c_str(), Current.size() + 1);
		if (!ImGui::InputText(Label, Buffer.GetData(), Buffer.Num())) return false;
		Field = Buffer.GetData();
		return true;
	}
}

FAnimGraphEditorTab::FAnimGraphEditorTab(const FString& InPath)
	: FEditorTab({ "AnimGraph - " + PathUtils::ToUtf8(PathUtils::FromUtf8(InPath).filename()) + "###" + InPath,
		"AnimGraphDock##" + InPath, true, true }), Path(InPath)
{
	Panel = AddPanel<FAnimGraphEditor>(Path);
}

void FAnimGraphEditorTab::Tick(float DeltaTime)
{
	FEditorTab::Tick(DeltaTime);
	if (Panel->WantsClose()) *GetOpenFlag() = false;
}

FAnimGraphEditor::FAnimGraphEditor(FString InPath)
	: Path(std::move(InPath)), PanelName("AnimGraph##" + Path)
{
}

FAnimGraphEditor::~FAnimGraphEditor()
{
	ImNodes::Ez::SetContext(nullptr);
	for (auto& Node : Nodes) if (Node->StateContext) ImNodes::Ez::FreeContext(Node->StateContext);
	ImNodes::Ez::FreeContext(Context);
}

bool FAnimGraphEditor::Init()
{
	Context = ImNodes::Ez::CreateContext();
	ImNodes::Ez::SetContext(nullptr);
	RefreshAssets();
	bLoaded = Load();
	return bLoaded;
}

void FAnimGraphEditor::RefreshAssets()
{
	Sequences.Empty(); BlendSpacePaths.Empty();
	for (TObjectIterator<UAnimSequence> It; It; ++It)
		if (!It->GetPath().empty() && It->GetDataModel()) Sequences.Add(*It);
	std::sort(Sequences.begin(), Sequences.end(), [](const auto* A, const auto* B) { return A->GetPath() < B->GetPath(); });
	std::error_code Error;
	const auto Root = std::filesystem::absolute("Assets", Error);
	if (Error) return;
	std::filesystem::recursive_directory_iterator It(Root, Error), End;
	while (!Error && It != End)
	{
		if (It->is_regular_file(Error) && It->path().extension() == ".blendspace")
			BlendSpacePaths.Add(PathUtils::ToUtf8(It->path().lexically_relative(Root)));
		It.increment(Error);
	}
	std::sort(BlendSpacePaths.begin(), BlendSpacePaths.end());
}

bool FAnimGraphEditor::Load()
{
	try
	{
		std::ifstream File(PathUtils::FromUtf8(Path));
		if (!File) throw std::runtime_error("Could not open the graph file.");
		File >> Document;
		if (Document.at("Version").get<int32>() != 1) throw std::runtime_error("Unsupported AnimGraph version.");
		auto& Graph = Document.at("AnimGraph");
		Graph.at("RootNodeID").get<int32>();
		for (const char* Name : { "Nodes", "Parameters", "AnimSequencePlayers", "AnimTwoWayBlends", "BlendSpacePlayers", "StateMachines" })
		{
			if (!Graph.contains(Name)) Graph[Name] = nlohmann::json::array();
			if (!Graph[Name].is_array()) throw std::runtime_error("Graph data fields must be arrays.");
		}
		for (const auto& Entry : Graph["Nodes"])
		{
			auto Node = MakeUnique<FNode>();
			Node->ID = Entry.at("NodeID").get<int32>();
			Node->Type = Entry.at("Type").get<int32>();
			Node->DataIndex = Entry.at("ActualType").get<int32>();
			if (Node->ID < 0 || Node->ID >= std::numeric_limits<int32>::max() - 1 || FindNode(Node->ID))
				throw std::runtime_error("Node IDs must be unique non-negative integers.");
			if (Node->Type < 0 || Node->Type > 3 || Node->DataIndex < 0
				|| Node->DataIndex >= static_cast<int32>(Graph[DataArrayName(Node->Type)].size()))
				throw std::runtime_error("Invalid node type or data index.");
			const auto& Value = Data(*Node);
			if (Node->Type == 0)
			{
				Value.at("SequencePath").get<FString>(); Value.at("Looping").get<bool>(); Value.at("Speed").get<float>();
			}
			else if (Node->Type == 1)
			{
				Value.at("NodeIDA").get<int32>(); Value.at("NodeIDB").get<int32>(); Value.at("Alpha").get<float>();
			}
			else if (Node->Type == 2)
			{
				Value.at("BlendSpacePath").get<FString>(); Value.at("Looping").get<bool>();
				Value.at("Speed").get<float>(); Value.at("X").get<float>(); Value.at("Y").get<float>();
			}
			else
			{
				Value.at("InitStateID").get<int32>();
				for (const char* Field : { "States", "Transitions", "AnyTransitions" })
					if (!Value.at(Field).is_array()) throw std::runtime_error("StateMachine fields must be arrays.");
				TSet<int32> IDs;
				for (const auto& State : Value.at("States"))
				{
					const int32 ID = State.at("StateID").get<int32>();
					if (ID < 0 || IDs.Contains(ID)) throw std::runtime_error("State IDs must be unique and non-negative.");
					IDs.Add(ID); State.at("Name").get<FString>(); State.at("NodeID").get<int32>();
				}
			}
			Node->Position = ImVec2(60.f + (Nodes.Num() % 3) * 250.f, 60.f + (Nodes.Num() / 3) * 170.f);
			if (Document.contains("Editor") && Document["Editor"].contains("Nodes"))
			{
				for (const auto& Layout : Document["Editor"]["Nodes"])
					if (Layout.value("NodeID", -1) == Node->ID)
						Node->Position = ImVec2(Layout.at("X").get<float>(), Layout.at("Y").get<float>());
			}
			NextID = std::max(NextID, Node->ID + 1);
			Nodes.Add(std::move(Node));
		}
		for (auto& Node : Nodes) if (Node->Type == 3) SyncStates(*Node);
		return true;
	}
	catch (const std::exception& Error)
	{
		Status = Error.what(); bStatusError = true;
		return false;
	}
}

FAnimGraphEditor::FNode* FAnimGraphEditor::FindNode(int32 ID) const
{
	for (const auto& Node : Nodes) if (Node->ID == ID) return Node.get();
	return nullptr;
}

nlohmann::json& FAnimGraphEditor::Data(FNode& Node)
{
	return Document["AnimGraph"][DataArrayName(Node.Type)][Node.DataIndex];
}

const nlohmann::json& FAnimGraphEditor::Data(const FNode& Node) const
{
	return Document.at("AnimGraph").at(DataArrayName(Node.Type)).at(Node.DataIndex);
}

FAnimGraphEditor::FStateView* FAnimGraphEditor::FindState(FNode& Machine, int32 ID) const
{
	if (ID == -1) return &Machine.AnyState;
	for (auto& State : Machine.States) if (State->ID == ID) return State.get();
	return nullptr;
}

void FAnimGraphEditor::SyncStates(FNode& Machine)
{
	if (!Machine.StateContext)
	{
		Machine.StateContext = ImNodes::Ez::CreateContext();
		ImNodes::Ez::SetContext(nullptr);
	}
	const bool bRestoreLayout = Machine.States.IsEmpty();
	Machine.AnyState.ID = -1;
	Machine.AnyState.Slot = "Any State";
	if (Machine.States.IsEmpty()) Machine.AnyState.Position = ImVec2(60.f, 50.f);
	for (const auto& State : Data(Machine)["States"])
	{
		const int32 ID = State.at("StateID").get<int32>();
		FStateView* View = FindState(Machine, ID);
		if (!View)
		{
			auto NewState = MakeUnique<FStateView>();
			NewState->ID = ID;
			NewState->Position = ImVec2(100.f + (Machine.States.Num() % 2) * 340.f, 200.f + (Machine.States.Num() / 2) * 230.f);
			View = NewState.get(); Machine.States.Add(std::move(NewState));
		}
		const FString Label = State.at("Name").get<FString>() + " [" + std::to_string(ID) + "]";
		if (View->Slot != Label) View->Slot = Label;
	}
	if (bRestoreLayout && Document.contains("Editor") && Document["Editor"].contains("States"))
		for (const auto& Layout : Document["Editor"]["States"])
			if (Layout.value("MachineNodeID", -1) == Machine.ID)
				if (FStateView* View = FindState(Machine, Layout.value("StateID", -2)))
					View->Position = ImVec2(Layout.at("X").get<float>(), Layout.at("Y").get<float>());
}

bool FAnimGraphEditor::CanReach(int32 StartID, int32 TargetID) const
{
	TArray<int32> Pending; Pending.Add(StartID);
	TSet<int32> Visited;
	while (!Pending.IsEmpty())
	{
		const int32 ID = Pending.Last(); Pending.RemoveLast();
		if (ID == TargetID) return true;
		if (Visited.Contains(ID)) continue;
		Visited.Add(ID);
		const FNode* Node = FindNode(ID);
		if (Node && Node->Type == 1)
		{
			Pending.Add(Data(*Node).at("NodeIDA").get<int32>());
			Pending.Add(Data(*Node).at("NodeIDB").get<int32>());
		}
		else if (Node && Node->Type == 3)
			for (const auto& State : Data(*Node).at("States")) Pending.Add(State.at("NodeID").get<int32>());
	}
	return false;
}

void FAnimGraphEditor::AcceptConnection(FNode* Input, const char* Slot, FNode* Output)
{
	if (!Input || !Output || !Slot || (Input->Type != 1 && Input->Type != 3)) return;
	if (Input->Type == 1 && std::strcmp(Slot, "A") && std::strcmp(Slot, "B")) return;
	if (CanReach(Output->ID, Input->ID))
	{
		Status = "A pose connection cannot form a cycle."; bStatusError = true; return;
	}
	if (Input->Type == 1) Data(*Input)[std::strcmp(Slot, "A") == 0 ? "NodeIDA" : "NodeIDB"] = Output->ID;
	else
	{
		bool bFound = false;
		for (auto& State : Data(*Input)["States"])
			if (FStateView* View = FindState(*Input, State["StateID"].get<int32>()))
				if (View->Slot == Slot) { State["NodeID"] = Output->ID; bFound = true; break; }
		if (!bFound) return;
	}
	bDirty = true; Status.clear();
}

void FAnimGraphEditor::AddNode(int32 Type)
{
	auto Node = MakeUnique<FNode>();
	Node->ID = NextID++; Node->Type = Type;
	auto& Graph = Document["AnimGraph"];
	auto& Values = Graph[DataArrayName(Type)];
	Node->DataIndex = static_cast<int32>(Values.size());
	if (Type == 0) Values.push_back({ {"SequencePath", ""}, {"Looping", true}, {"Speed", 1.f} });
	else if (Type == 1) Values.push_back({ {"NodeIDA", -1}, {"NodeIDB", -1}, {"Alpha", 0.5f} });
	else if (Type == 2) Values.push_back({ {"BlendSpacePath", ""}, {"Looping", true}, {"Speed", 1.f}, {"X", 0.f}, {"Y", 0.f} });
	else Values.push_back({ {"InitStateID", -1}, {"States", nlohmann::json::array()},
		{"AnyTransitions", nlohmann::json::array()}, {"Transitions", nlohmann::json::array()} });
	Graph["Nodes"].push_back({ {"NodeID", Node->ID}, {"Type", Type}, {"ActualType", Node->DataIndex} });
	for (auto& Other : Nodes) Other->bSelected = false;
	SelectedID = Node->ID; Node->bSelected = true;
	Node->Position = ImVec2(60.f + (Nodes.Num() % 3) * 250.f, 60.f + (Nodes.Num() / 3) * 170.f);
	if (Graph["RootNodeID"].get<int32>() < 0) Graph["RootNodeID"] = Node->ID;
	if (Type == 3) SyncStates(*Node);
	Nodes.Add(std::move(Node)); bDirty = true;
}

void FAnimGraphEditor::DeleteSelectedNode()
{
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		if (Nodes[Index]->ID != SelectedID) continue;
		auto& Graph = Document["AnimGraph"];
		auto& Entries = Graph["Nodes"];
		for (auto It = Entries.begin(); It != Entries.end(); ++It)
			if (It->at("NodeID").get<int32>() == SelectedID) { Entries.erase(It); break; }
		for (auto& Other : Nodes)
		{
			if (Other->Type == 3)
				for (auto& State : Data(*Other)["States"])
					if (State["NodeID"].get<int32>() == SelectedID) State["NodeID"] = -1;
			if (Other->Type != 1) continue;
			for (const char* Field : { "NodeIDA", "NodeIDB" })
				if (Data(*Other)[Field].get<int32>() == SelectedID) Data(*Other)[Field] = -1;
		}
		if (Graph["RootNodeID"].get<int32>() == SelectedID) Graph["RootNodeID"] = -1;
		if (ViewedMachineID == SelectedID) ViewedMachineID = -1;
		if (Nodes[Index]->StateContext) ImNodes::Ez::FreeContext(Nodes[Index]->StateContext);
		Nodes.RemoveAt(Index); SelectedID = -1; bDirty = true;
		break;
	}
}

bool FAnimGraphEditor::ValidateForSave()
{
	TSet<FString> ParameterNames;
	for (const auto& Parameter : Document["AnimGraph"]["Parameters"])
	{
		const FString Name = Parameter.at("Name").get<FString>();
		if (Name.empty() || ParameterNames.Contains(Name)) { Status = "Parameter names must be unique and non-empty."; return false; }
		ParameterNames.Add(Name);
	}
	const int32 Root = Document["AnimGraph"]["RootNodeID"].get<int32>();
	if (!Nodes.IsEmpty() && !FindNode(Root)) { Status = "Choose a Root node before saving."; return false; }
	for (const auto& Node : Nodes)
	{
		if (Node->Type == 2 && Data(*Node)["BlendSpacePath"].get<FString>().empty())
		{ Status = "Choose a BlendSpace for every BlendSpace Player."; return false; }
		if (Node->Type == 3)
		{
			auto& Machine = Data(*Node);
			TSet<int32> StateIDs;
			for (const auto& State : Machine["States"])
			{
				const int32 ID = State.at("StateID").get<int32>();
				const int32 PoseID = State.at("NodeID").get<int32>();
				if (ID < 0 || StateIDs.Contains(ID)) { Status = "State IDs must be unique."; return false; }
				StateIDs.Add(ID);
				if (!FindNode(PoseID)) { Status = "Connect a Pose to every state."; return false; }
				if (CanReach(PoseID, Node->ID)) { Status = "State Pose connection contains a cycle."; return false; }
			}
			if (!StateIDs.Contains(Machine["InitStateID"].get<int32>()))
			{ Status = "Choose a valid entry state."; return false; }
			for (const char* Field : { "AnyTransitions", "Transitions" })
				for (const auto& Transition : Machine[Field])
				{
					if (!StateIDs.Contains(Transition.at("NextStateID").get<int32>()) ||
						(std::strcmp(Field, "Transitions") == 0 && !StateIDs.Contains(Transition.at("PreviousStateID").get<int32>())))
					{ Status = "Transition references a missing state."; return false; }
					if (!ParameterNames.Contains(Transition.at("Condition").at("ParameterName").get<FString>()))
					{ Status = "Select a valid parameter for every transition."; return false; }
					const float Duration = Transition.at("CrossfadeDuration").get<float>();
					if (!std::isfinite(Duration) || Duration < 0.f) { Status = "Crossfade duration must be non-negative."; return false; }
					const float ExitTime = Transition.value("ExitTime", 1.f);
					if (Transition.value("HasExitTime", false) && (!std::isfinite(ExitTime) || ExitTime < 0.f || ExitTime > 1.f))
					{ Status = "Exit time must be between 0 and 1."; return false; }
				}
		}
		if (Node->Type == 0 && Data(*Node)["SequencePath"].get<FString>().empty())
		{ Status = "Choose a Sequence for every Sequence Player."; return false; }
		if (Node->Type == 1)
		{
			for (const char* Field : { "NodeIDA", "NodeIDB" })
			{
				const int32 Input = Data(*Node)[Field].get<int32>();
				if (!FindNode(Input)) { Status = "Connect both A and B inputs before saving."; return false; }
				if (CanReach(Input, Node->ID)) { Status = "Graph contains a pose cycle."; return false; }
			}
		}
	}
	return true;
}

bool FAnimGraphEditor::Save()
{
	bStatusError = true;
	if (!ValidateForSave()) return false;
	std::filesystem::path TemporaryPath;
	try
	{
		auto& Layout = Document["Editor"]["Nodes"];
		Layout = nlohmann::json::array();
		for (const auto& Node : Nodes) Layout.push_back({ {"NodeID", Node->ID}, {"X", Node->Position.x}, {"Y", Node->Position.y} });
		auto& StateLayout = Document["Editor"]["States"];
		StateLayout = nlohmann::json::array();
		for (const auto& Node : Nodes) if (Node->Type == 3)
		{
			for (const auto& State : Node->States)
				StateLayout.push_back({ {"MachineNodeID", Node->ID}, {"StateID", State->ID}, {"X", State->Position.x}, {"Y", State->Position.y} });
			StateLayout.push_back({ {"MachineNodeID", Node->ID}, {"StateID", -1}, {"X", Node->AnyState.Position.x}, {"Y", Node->AnyState.Position.y} });
		}
		const auto Destination = PathUtils::FromUtf8(Path);
		TemporaryPath = Destination; TemporaryPath += L".tmp";
		for (int32 Index = 1; std::filesystem::exists(TemporaryPath); ++Index)
			TemporaryPath = Destination.wstring() + L".tmp" + std::to_wstring(Index);
		std::ofstream File(TemporaryPath, std::ios::binary);
		const FString Text = Document.dump(4);
		File.write(Text.data(), static_cast<std::streamsize>(Text.size())); File.flush();
		if (!File) throw std::runtime_error("Could not save the graph file.");
		File.close();
		if (!File || !MoveFileExW(TemporaryPath.c_str(), Destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			throw std::runtime_error("Could not replace the graph file; original file was kept.");
		bDirty = false; bStatusError = false; Status = "Saved.";
		return true;
	}
	catch (const std::exception& Error)
	{
		if (!TemporaryPath.empty()) { std::error_code Ignore; std::filesystem::remove(TemporaryPath, Ignore); }
		Status = Error.what(); return false;
	}
}

bool FAnimGraphEditor::ConfirmClose()
{
	if (!bDirty || bCloseApproved) return true;
	SetOpen(true);
	bCloseRequested = true;
	return false;
}

void FAnimGraphEditor::DrawCloseConfirmation()
{
	if (bCloseRequested) { ImGui::OpenPopup("Unsaved graph"); bCloseRequested = false; }
	if (ImGui::BeginPopupModal("Unsaved graph", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Save changes before closing?");
		if (ImGui::Button("Save and Close"))
		{
			if (Save()) { bCloseApproved = true; ImGui::CloseCurrentPopup(); }
		}
		ImGui::SameLine();
		if (ImGui::Button("Discard")) { bCloseApproved = true; ImGui::CloseCurrentPopup(); }
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		if (bStatusError && !Status.empty()) ImGui::TextWrapped("%s", Status.c_str());
		ImGui::EndPopup();
	}
}

void FAnimGraphEditor::DrawToolbar()
{
	if (ViewedMachineID >= 0)
	{
		if (ImGui::Button("< AnimGraph")) { ViewedMachineID = -1; SelectedStateID = -1; }
		ImGui::SameLine(); ImGui::Text("StateMachine %d", ViewedMachineID);
	}
	if (ImGui::Button("Save")) Save();
	ImGui::SameLine();
	ImGui::TextDisabled("%s", bDirty ? "Unsaved changes" : "Saved");
	ImGui::TextWrapped("%s", Path.c_str());
	ImGui::TextDisabled("Right-click to add. Double-click StateMachine to open states. Double-click a link to remove it.");
	if (!Status.empty()) ImGui::TextColored(bStatusError ? ImVec4(1.f,.4f,.3f,1.f) : ImVec4(.4f,.9f,.5f,1.f), "%s", Status.c_str());
}

void FAnimGraphEditor::DrawCanvas()
{
	ImNodes::Ez::SetContext(Context);
	if (FNode* Machine = FindNode(ViewedMachineID))
	{
		ImNodes::Ez::SetContext(Machine->StateContext);
		DrawStateCanvas(*Machine);
		ImNodes::Ez::SetContext(nullptr);
		return;
	}
	ImNodes::Ez::BeginCanvas();
	const ImNodes::Ez::SlotInfo Inputs[] = { {"A", 1}, {"B", 1} };
	const ImNodes::Ez::SlotInfo Output[] = { {"Pose", 1} };
	bool bNodeHovered = false;
	for (auto& Node : Nodes)
	{
		const ImVec2 OldPosition = Node->Position;
		if (ImNodes::Ez::BeginNode(Node.get(), NodeTitle(Node->Type), &Node->Position, &Node->bSelected))
		{
			TArray<ImNodes::Ez::SlotInfo> StateInputs;
			if (Node->Type == 3)
				for (const auto& State : Node->States) StateInputs.Add({ State->Slot.c_str(), 1 });
			if (Node->Type == 3) ImNodes::Ez::InputSlots(StateInputs.GetData(), StateInputs.Num());
			else ImNodes::Ez::InputSlots(Node->Type == 1 ? Inputs : nullptr, Node->Type == 1 ? 2 : 0);
			ImGui::Text("ID %d%s", Node->ID, Document["AnimGraph"]["RootNodeID"].get<int32>() == Node->ID ? "  [ROOT]" : "");
			if (Node->Type == 0)
			{
				const FString Clip = Data(*Node)["SequencePath"].get<FString>();
				ImGui::TextUnformatted(Clip.empty() ? "Select a Sequence" : PathUtils::ToUtf8(PathUtils::FromUtf8(Clip).filename()).c_str());
			}
			else if (Node->Type == 1) ImGui::Text("Alpha: %.2f", Data(*Node)["Alpha"].get<float>());
			else if (Node->Type == 2)
			{
				const FString Space = Data(*Node)["BlendSpacePath"].get<FString>();
				ImGui::TextUnformatted(Space.empty() ? "Select BlendSpace" : PathUtils::ToUtf8(PathUtils::FromUtf8(Space).filename()).c_str());
				ImGui::Text("X: %.2f", Data(*Node)["X"].get<float>());
			}
			else { ImGui::Text("States: %d", Node->States.Num()); ImGui::TextDisabled("Double-click to open"); }
			if (Node->Type == 1)
				ImGui::Dummy(ImVec2(110.f * ImNodes::Ez::GetState().Zoom, 0.f));
			ImNodes::Ez::OutputSlots(Output, 1);
			bNodeHovered |= ImNodes::IsNodeHovered();
			if (ImNodes::IsNodeHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) SelectedID = Node->ID;
			if (Node->Type == 3 && ImNodes::IsNodeHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{ ViewedMachineID = Node->ID; SelectedStateID = -1; }
			void* InputNode = nullptr; void* OutputNode = nullptr;
			const char* InputSlot = nullptr; const char* OutputSlot = nullptr;
			if (ImNodes::GetNewConnection(&InputNode, &InputSlot, &OutputNode, &OutputSlot))
				AcceptConnection(static_cast<FNode*>(InputNode), InputSlot, static_cast<FNode*>(OutputNode));
		}
		ImNodes::Ez::EndNode();
		if (Node->Position.x != OldPosition.x || Node->Position.y != OldPosition.y) bDirty = true;
	}
	for (auto& Node : Nodes)
	{
		if (Node->Type == 3)
			for (auto& State : Data(*Node)["States"])
				if (FNode* Pose = FindNode(State["NodeID"].get<int32>()))
					if (FStateView* View = FindState(*Node, State["StateID"].get<int32>()))
						if (!ImNodes::Ez::Connection(Node.get(), View->Slot.c_str(), Pose, "Pose"))
						{ State["NodeID"] = -1; bDirty = true; }
		if (Node->Type != 1) continue;
		for (const char* Slot : { "A", "B" })
		{
			const char* Field = Slot[0] == 'A' ? "NodeIDA" : "NodeIDB";
			if (FNode* Input = FindNode(Data(*Node)[Field].get<int32>()))
				if (!ImNodes::Ez::Connection(Node.get(), Slot, Input, "Pose")) { Data(*Node)[Field] = -1; bDirty = true; }
		}
	}
	ImNodes::Ez::EndCanvas();
	if (!bNodeHovered && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		const auto& Canvas = ImNodes::Ez::GetState();
		const ImVec2 Mouse = ImGui::GetMousePos();
		const ImVec2 Origin = ImGui::GetWindowPos();
		NewNodePosition = ImVec2((Mouse.x - Origin.x - Canvas.Offset.x) / Canvas.Zoom,
			(Mouse.y - Origin.y - Canvas.Offset.y) / Canvas.Zoom);
		ImGui::OpenPopup("Add Node");
	}
	if (ImGui::BeginPopup("Add Node"))
	{
		int32 Type = -1;
		if (ImGui::MenuItem("Sequence Player")) Type = 0;
		if (ImGui::MenuItem("Two Way Blend")) Type = 1;
		if (ImGui::MenuItem("BlendSpace Player")) { Type = 2; RefreshAssets(); }
		if (ImGui::MenuItem("State Machine")) Type = 3;
		if (Type >= 0)
		{
			AddNode(Type);
			if (FNode* Node = FindNode(SelectedID)) Node->Position = NewNodePosition;
		}
		ImGui::EndPopup();
	}
	ImNodes::Ez::SetContext(nullptr);
}

void FAnimGraphEditor::AddTransition(FNode& Machine, int32 From, int32 To)
{
	if (To < 0 || From == To || !FindState(Machine, From) || !FindState(Machine, To)) return;
	const char* Field = From == -1 ? "AnyTransitions" : "Transitions";
	for (const auto& Transition : Data(Machine)[Field])
		if (Transition["PreviousStateID"] == From && Transition["NextStateID"] == To) return;
	auto& Parameters = Document["AnimGraph"]["Parameters"];
	if (Parameters.empty()) Parameters.push_back({ {"Name", "Action"}, {"Type", 0}, {"Value", 0} });
	const auto& Parameter = Parameters[0];
	const int32 Type = Parameter["Type"].get<int32>();
	nlohmann::json Value = (Type == 1 || Type == 3) ? nlohmann::json(true) : Type == 2 ? nlohmann::json(0.f) : nlohmann::json(To);
	Data(Machine)[Field].push_back({ {"PreviousStateID", From}, {"NextStateID", To}, {"CrossfadeDuration", .2f},
		{"HasExitTime", false}, {"ExitTime", 1.f},
		{"Condition", { {"ParameterName", Parameter["Name"]}, {"ComparisonOperator", 0}, {"Value", Value} }} });
	bDirty = true;
}

bool FAnimGraphEditor::DrawStateNode(FNode& Machine, FStateView& State)
{
	bool bHovered = false;
	const ImVec2 OldPosition = State.Position;
	const ImNodes::Ez::SlotInfo Input[] = { {"Enter", 2} }, Output[] = { {"Transition", 2} };
	if (ImNodes::Ez::BeginNode(&State, State.Slot.c_str(), &State.Position, &State.bSelected))
	{
		ImNodes::Ez::InputSlots(State.ID >= 0 ? Input : nullptr, State.ID >= 0 ? 1 : 0);
		if (State.ID >= 0)
		{
			if (Data(Machine)["InitStateID"] == State.ID) ImGui::TextUnformatted("[ENTRY]");
			for (const auto& Entry : Data(Machine)["States"])
				if (Entry["StateID"] == State.ID)
				{
					FNode* Pose = FindNode(Entry["NodeID"].get<int32>());
					ImGui::Text("Pose: %s (%d)", Pose ? NodeTitle(Pose->Type) : "Unconnected", Entry["NodeID"].get<int32>());
				}
		}
		else ImGui::TextDisabled("Global transitions");
		ImNodes::Ez::OutputSlots(Output, 1);
		bHovered = ImNodes::IsNodeHovered();
		if (ImNodes::IsNodeHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) SelectedStateID = State.ID;
		void* InputNode = nullptr; void* OutputNode = nullptr;
		const char* InputSlot = nullptr; const char* OutputSlot = nullptr;
		if (ImNodes::GetNewConnection(&InputNode, &InputSlot, &OutputNode, &OutputSlot))
			if (InputNode && OutputNode && InputSlot && OutputSlot &&
				!std::strcmp(InputSlot, "Enter") && !std::strcmp(OutputSlot, "Transition"))
				AddTransition(Machine, static_cast<FStateView*>(OutputNode)->ID, static_cast<FStateView*>(InputNode)->ID);
	}
	ImNodes::Ez::EndNode();
	if (State.Position.x != OldPosition.x || State.Position.y != OldPosition.y) bDirty = true;
	return bHovered;
}

void FAnimGraphEditor::DrawStateCanvas(FNode& Machine)
{
	// Canvas-local IDs keep state slot caches separate from the outer pose graph.
	ImGui::PushID(Machine.ID);
	ImNodes::Ez::BeginCanvas();
	bool bNodeHovered = DrawStateNode(Machine, Machine.AnyState);
	for (auto& State : Machine.States) bNodeHovered |= DrawStateNode(Machine, *State);
	for (const char* Field : { "AnyTransitions", "Transitions" })
	{
		auto& Transitions = Data(Machine)[Field];
		int32 Remove = -1;
		TSet<FString> Drawn;
		for (int32 i = 0; i < static_cast<int32>(Transitions.size()); ++i)
		{
			auto& Transition = Transitions[i];
			const int32 FromID = std::strcmp(Field, "AnyTransitions") == 0 ? -1 : Transition["PreviousStateID"].get<int32>();
			const int32 ToID = Transition["NextStateID"].get<int32>();
			FStateView* From = FindState(Machine, FromID);
			FStateView* To = FindState(Machine, ToID);
			const FString Key = std::to_string(FromID) + ":" + std::to_string(ToID);
			if (!From || !To || ToID < 0 || Drawn.Contains(Key)) continue;
			Drawn.Add(Key);
			if (!ImNodes::Ez::Connection(To, "Enter", From, "Transition")) Remove = i;
		}
		if (Remove >= 0) { Transitions.erase(Transitions.begin() + Remove); bDirty = true; }
	}
	ImNodes::Ez::EndCanvas();
	if (!bNodeHovered && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		const auto& Canvas = ImNodes::Ez::GetState();
		const ImVec2 Mouse = ImGui::GetMousePos(), Origin = ImGui::GetWindowPos();
		NewNodePosition = ImVec2((Mouse.x - Origin.x - Canvas.Offset.x) / Canvas.Zoom,
			(Mouse.y - Origin.y - Canvas.Offset.y) / Canvas.Zoom);
		ImGui::OpenPopup("Add State");
	}
	if (ImGui::BeginPopup("Add State"))
	{
		if (ImGui::MenuItem("State")) AddState(Machine, NewNodePosition);
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void FAnimGraphEditor::AddState(FNode& Machine, const ImVec2& Position)
{
	auto& Value = Data(Machine);
	int32 ID = 0;
	for (const auto& State : Machine.States) ID = std::max(ID, State->ID + 1);
	Value["States"].push_back({ {"StateID", ID}, {"Name", "State " + std::to_string(ID)}, {"NodeID", -1} });
	SyncStates(Machine);
	FindState(Machine, ID)->Position = Position;
	if (Value["InitStateID"].get<int32>() < 0) Value["InitStateID"] = ID;
	SelectedStateID = ID; bDirty = true;
}

void FAnimGraphEditor::DrawStateCombo(const char* Label, FNode& Machine, nlohmann::json& Field)
{
	const int32 ID = Field.get<int32>();
	FStateView* Current = ID >= 0 ? FindState(Machine, ID) : nullptr;
	if (ImGui::BeginCombo(Label, Current ? Current->Slot.c_str() : "Select state"))
	{
		for (const auto& State : Machine.States)
			if (ImGui::Selectable(State->Slot.c_str(), ID == State->ID)) { Field = State->ID; bDirty = true; }
		ImGui::EndCombo();
	}
}

void FAnimGraphEditor::DrawTransitions(FNode& Machine, const char* Field)
{
	ImGui::PushID(Field);
	const bool bAny = !std::strcmp(Field, "AnyTransitions");
	auto& Transitions = Data(Machine)[Field];
	int32 Remove = -1;
	const char* Operators[] = { "==", "!=", ">", "<", ">=", "<=" };
	for (int32 i = 0; i < static_cast<int32>(Transitions.size()); ++i)
	{
		auto& Transition = Transitions[i];
		ImGui::PushID(i);
		FStateView* From = FindState(Machine, bAny ? -1 : Transition["PreviousStateID"].get<int32>());
		FStateView* To = FindState(Machine, Transition["NextStateID"].get<int32>());
		const FString Title = (From ? From->Slot : "Missing") + " -> " + (To ? To->Slot : "Missing");
		if (ImGui::TreeNode("Transition", "%s", Title.c_str()))
		{
			if (!bAny) DrawStateCombo("From", Machine, Transition["PreviousStateID"]);
			DrawStateCombo("To", Machine, Transition["NextStateID"]);
			float Duration = Transition["CrossfadeDuration"].get<float>();
			if (ImGui::DragFloat("Crossfade (s)", &Duration, .01f, 0.f, 10.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			{ Transition["CrossfadeDuration"] = Duration; bDirty = true; }
			bool bHasExitTime = Transition.value("HasExitTime", false);
			if (ImGui::Checkbox("Has Exit Time", &bHasExitTime))
			{
				Transition["HasExitTime"] = bHasExitTime;
				Transition["ExitTime"] = Transition.value("ExitTime", 1.f);
				bDirty = true;
			}
			ImGui::BeginDisabled(!bHasExitTime);
			float ExitTime = Transition.value("ExitTime", 1.f);
			if (ImGui::DragFloat("Exit Time", &ExitTime, .01f, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			{ Transition["ExitTime"] = ExitTime; bDirty = true; }
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("Normalized playback time: 0 = start, 1 = end. Parameter conditions still apply.");
			auto& Condition = Transition["Condition"];
			FString Name = Condition["ParameterName"].get<FString>();
			if (ImGui::BeginCombo("Parameter", Name.empty() ? "Select parameter" : Name.c_str()))
			{
				for (const auto& Parameter : Document["AnimGraph"]["Parameters"])
				{
					const FString Candidate = Parameter["Name"].get<FString>();
					if (ImGui::Selectable(Candidate.c_str(), Candidate == Name))
					{
						Condition["ParameterName"] = Candidate; Name = Candidate;
						const int32 Type = Parameter["Type"].get<int32>();
						Condition["Value"] = (Type == 1 || Type == 3) ? nlohmann::json(false) : Type == 2 ? nlohmann::json(0.f) : nlohmann::json(0);
						bDirty = true;
					}
				}
				ImGui::EndCombo();
			}
			int32 Type = -1;
			for (const auto& Parameter : Document["AnimGraph"]["Parameters"])
				if (Parameter["Name"] == Name) Type = Parameter["Type"].get<int32>();
			if (Type != 3)
			{
				int32 Operator = Condition["ComparisonOperator"].get<int32>();
				if (ImGui::Combo("Compare", &Operator, Operators, 6)) { Condition["ComparisonOperator"] = Operator; bDirty = true; }
			}
			else ImGui::TextDisabled("Transitions when this Trigger is set; consumed on transition.");
			if (Type == 0)
			{
				int32 Value = Condition["Value"].get<int32>();
				if (ImGui::InputInt("Value", &Value)) { Condition["Value"] = Value; bDirty = true; }
			}
			else if (Type == 1)
			{
				bool Value = Condition["Value"].get<bool>();
				if (ImGui::Checkbox("Value", &Value)) { Condition["Value"] = Value; bDirty = true; }
			}
			else if (Type == 2)
			{
				float Value = Condition["Value"].get<float>();
				if (ImGui::DragFloat("Value", &Value, .1f)) { Condition["Value"] = Value; bDirty = true; }
			}
			if (ImGui::Button("Delete Transition")) Remove = i;
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	if (Remove >= 0) { Transitions.erase(Transitions.begin() + Remove); bDirty = true; }
	ImGui::PopID();
}

void FAnimGraphEditor::DrawStateMachineDetails(FNode& Machine)
{
	if (ImGui::Button("Open StateMachine")) { ViewedMachineID = Machine.ID; SelectedStateID = -1; }
	auto& Value = Data(Machine);
	DrawStateCombo("Entry State", Machine, Value["InitStateID"]);
	ImGui::TextWrapped("Right-click the StateMachine canvas to add a state.");
	int32 RemoveID = -1;
	for (auto& State : Value["States"])
	{
		const int32 ID = State["StateID"].get<int32>();
		FStateView* View = FindState(Machine, ID);
		ImGui::PushID(ID);
		if (ViewedMachineID == Machine.ID && SelectedStateID == ID) ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("State", "%s", View->Slot.c_str()))
		{
			if (EditGraphText("Name", State["Name"]))
			{ View->Slot = State["Name"].get<FString>() + " [" + std::to_string(ID) + "]"; bDirty = true; }
			FNode* Pose = FindNode(State["NodeID"].get<int32>());
			if (ImGui::BeginCombo("Pose Node", Pose ? (FString(NodeTitle(Pose->Type)) + " #" + std::to_string(Pose->ID)).c_str() : "Unconnected"))
			{
				for (const auto& Candidate : Nodes)
				{
					if (CanReach(Candidate->ID, Machine.ID)) continue;
					const FString Label = FString(NodeTitle(Candidate->Type)) + " #" + std::to_string(Candidate->ID);
					if (ImGui::Selectable(Label.c_str(), Pose == Candidate.get())) { State["NodeID"] = Candidate->ID; bDirty = true; }
				}
				ImGui::EndCombo();
			}
			if (ImGui::Button("Set Entry")) { Value["InitStateID"] = ID; bDirty = true; }
			ImGui::SameLine(); if (ImGui::Button("Delete State")) RemoveID = ID;
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	if (RemoveID >= 0)
	{
		auto& States = Value["States"];
		for (auto It = States.begin(); It != States.end(); ++It) if ((*It)["StateID"] == RemoveID) { States.erase(It); break; }
		for (int32 i = 0; i < Machine.States.Num(); ++i) if (Machine.States[i]->ID == RemoveID) { Machine.States.RemoveAt(i); break; }
		for (const char* Field : { "AnyTransitions", "Transitions" })
		{
			auto& Transitions = Value[Field];
			for (auto It = Transitions.begin(); It != Transitions.end();)
				if ((*It)["PreviousStateID"] == RemoveID || (*It)["NextStateID"] == RemoveID) It = Transitions.erase(It);
				else ++It;
		}
		if (Value["InitStateID"] == RemoveID) Value["InitStateID"] = States.empty() ? -1 : States[0]["StateID"].get<int32>();
		SelectedStateID = -1; bDirty = true;
	}
	ImGui::SeparatorText("Transitions");
	ImGui::TextWrapped("Connect Transition to Enter in the state canvas to add a rule. Edit rules below.");
	DrawTransitions(Machine, "AnyTransitions");
	DrawTransitions(Machine, "Transitions");
}

void FAnimGraphEditor::DrawParameters()
{
	if (!ImGui::CollapsingHeader("Parameters")) return;
	auto& Parameters = Document["AnimGraph"]["Parameters"];
	int32 Remove = -1;
	const char* Types[] = { "Int", "Bool", "Float", "Trigger" };
	for (int32 i = 0; i < static_cast<int32>(Parameters.size()); ++i)
	{
		auto& Parameter = Parameters[i];
		ImGui::PushID(i);
		const FString PreviousName = Parameter["Name"].get<FString>();
		const int32 PreviousType = Parameter["Type"].get<int32>();
		bool bChanged = EditGraphText("Name", Parameter["Name"]);
		int32 Type = PreviousType;
		if (ImGui::Combo("Type", &Type, Types, 4))
		{
			Parameter["Type"] = Type;
			Parameter["Value"] = (Type == 1 || Type == 3) ? nlohmann::json(false) : Type == 2 ? nlohmann::json(0.f) : nlohmann::json(0);
			bChanged = true;
		}
		if (Type == 0)
		{
			int32 Value = Parameter["Value"].get<int32>();
			if (ImGui::InputInt("Default", &Value)) { Parameter["Value"] = Value; bDirty = true; }
		}
		else if (Type == 1)
		{
			bool Value = Parameter["Value"].get<bool>();
			if (ImGui::Checkbox("Default", &Value)) { Parameter["Value"] = Value; bDirty = true; }
		}
		else if (Type == 2)
		{
			float Value = Parameter["Value"].get<float>();
			if (ImGui::DragFloat("Default", &Value, .1f)) { Parameter["Value"] = Value; bDirty = true; }
		}
		else if (Type == 3) ImGui::TextDisabled("Trigger starts unset; set by gameplay and consumed on transition.");
		bool bReferenced = false;
		for (auto& Machine : Document["AnimGraph"]["StateMachines"])
			for (const char* Field : { "AnyTransitions", "Transitions" })
				for (auto& Transition : Machine[Field])
				{
					auto& Condition = Transition["Condition"];
					if (Condition["ParameterName"] != PreviousName) continue;
					bReferenced = true;
					if (bChanged)
					{
						Condition["ParameterName"] = Parameter["Name"];
						if (PreviousType != Type) Condition["Value"] = Parameter["Value"];
					}
				}
		if (bChanged) bDirty = true;
		ImGui::BeginDisabled(bReferenced);
		if (ImGui::Button("Delete Parameter")) Remove = i;
		ImGui::EndDisabled();
		if (bReferenced) ImGui::TextDisabled("Used by transition rules");
		ImGui::Separator(); ImGui::PopID();
	}
	if (Remove >= 0) { Parameters.erase(Parameters.begin() + Remove); bDirty = true; }
	if (ImGui::Button("Add Parameter"))
	{
		int32 Index = static_cast<int32>(Parameters.size());
		FString Name;
		bool bExists;
		do
		{
			Name = "Parameter" + std::to_string(Index++); bExists = false;
			for (const auto& Parameter : Parameters) if (Parameter["Name"] == Name) bExists = true;
		} while (bExists);
		Parameters.push_back({ {"Name", Name}, {"Type", 0}, {"Value", 0} }); bDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Trigger"))
	{
		int32 Index = static_cast<int32>(Parameters.size());
		FString Name;
		bool bExists;
		do
		{
			Name = "Trigger" + std::to_string(Index++); bExists = false;
			for (const auto& Parameter : Parameters) if (Parameter["Name"] == Name) bExists = true;
		} while (bExists);
		Parameters.push_back({ {"Name", Name}, {"Type", 3}, {"Value", false} }); bDirty = true;
	}
}

void FAnimGraphEditor::DrawDetails()
{
	DrawParameters();
	ImGui::Separator();
	if (ViewedMachineID >= 0) SelectedID = ViewedMachineID;
	FNode* Node = FindNode(SelectedID);
	if (!Node) { ImGui::TextDisabled("Select a node"); return; }
	ImGui::TextUnformatted(NodeTitle(Node->Type));
	ImGui::Text("Node ID: %d", Node->ID);
	if (ImGui::Button("Set as Root")) { Document["AnimGraph"]["RootNodeID"] = Node->ID; bDirty = true; }
	auto& Value = Data(*Node);
	if (Node->Type == 0)
	{
		FString Clip = Value["SequencePath"].get<FString>();
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::BeginCombo("##Sequence", Clip.empty() ? "Select Sequence" : Clip.c_str()))
		{
			if (ImGui::IsWindowAppearing()) RefreshAssets();
			for (UAnimSequence* Sequence : Sequences)
				if (ImGui::Selectable(Sequence->GetPath().c_str(), Clip == Sequence->GetPath()))
				{ Value["SequencePath"] = Sequence->GetPath(); bDirty = true; }
			ImGui::EndCombo();
		}
		bool bLooping = Value["Looping"].get<bool>();
		if (ImGui::Checkbox("Loop", &bLooping)) { Value["Looping"] = bLooping; bDirty = true; }
		float Speed = Value["Speed"].get<float>();
		if (ImGui::DragFloat("Speed", &Speed, .01f, .01f, 10.f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) { Value["Speed"] = Speed; bDirty = true; }
	}
	else if (Node->Type == 1)
	{
		float Alpha = Value["Alpha"].get<float>();
		if (ImGui::SliderFloat("Alpha", &Alpha, 0.f, 1.f)) { Value["Alpha"] = Alpha; bDirty = true; }
		ImGui::Text("A: %d  B: %d", Value["NodeIDA"].get<int32>(), Value["NodeIDB"].get<int32>());
	}
	else if (Node->Type == 2)
	{
		const FString Current = Value["BlendSpacePath"].get<FString>();
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::BeginCombo("##BlendSpace", Current.empty() ? "Select BlendSpace" : Current.c_str()))
		{
			if (ImGui::IsWindowAppearing()) RefreshAssets();
			for (const FString& Space : BlendSpacePaths)
				if (ImGui::Selectable(Space.c_str(), Space == Current)) { Value["BlendSpacePath"] = Space; bDirty = true; }
			ImGui::EndCombo();
		}
		bool bLooping = Value["Looping"].get<bool>();
		if (ImGui::Checkbox("Loop", &bLooping)) { Value["Looping"] = bLooping; bDirty = true; }
		float Speed = Value["Speed"].get<float>(), X = Value["X"].get<float>(), Y = Value["Y"].get<float>();
		if (ImGui::DragFloat("Speed", &Speed, .01f, .01f, 10.f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) { Value["Speed"] = Speed; bDirty = true; }
		if (ImGui::DragFloat("X", &X, 1.f)) { Value["X"] = X; bDirty = true; }
		if (ImGui::DragFloat("Y", &Y, 1.f)) { Value["Y"] = Y; bDirty = true; }
	}
	else DrawStateMachineDetails(*Node);
	ImGui::Separator();
	if (ImGui::Button("Delete Node")) DeleteSelectedNode();
}

void FAnimGraphEditor::OnRender()
{
	ImGuiID PropertiesDockID = 0;
	// The owning document tab controls lifetime; closing this panel only hides its window.
	if (ImGui::Begin(GetPanelName()))
	{
		if (ImGui::IsWindowAppearing()) RefreshAssets();
		PropertiesDockID = ImGui::GetWindowDockID();
		if (!bLoaded) ImGui::TextWrapped("Could not open AnimGraph: %s", Status.c_str());
		else
		{
			DrawToolbar();
			ImGui::Separator();
			if (ImGui::BeginChild("Canvas", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders,
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) DrawCanvas();
			ImGui::EndChild();
		}
		DrawCloseConfirmation();
	}
	ImGui::End();
	if (bLoaded)
	{
		const FString PropertiesName = "AnimGraph Properties##" + Path;
		ImGuiWindowClass PropertiesClass;
		PropertiesClass.ClassId = ImHashStr(("AnimGraphDock##" + Path).c_str());
		PropertiesClass.DockingAllowUnclassed = false;
		ImGui::SetNextWindowClass(&PropertiesClass);
		if (PropertiesDockID != 0)
			ImGui::SetNextWindowDockID(PropertiesDockID, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(320.f, 400.f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(PropertiesName.c_str())) DrawDetails();
		ImGui::End();
	}
}

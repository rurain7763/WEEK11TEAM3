#include "EnginePCH.h"
#include "Animation/Graph/AnimGraph.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimNodes/AnimNode_SequencePlayer.h"
#include "Animation/AnimNodes/AnimNode_TwoWayBlend.h"
#include "Animation/AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "Animation/AnimNodes/AnimNode_StateMachine.h"
#include "Asset/AssetManager.h"
#include <algorithm>

template <typename T>
void UAnimGraph::SerializeArray(FStructuredArchiveRecord& Parent, const char* Name, TArray<T>& Values)
{
	FArchive& Archive = Parent.GetUnderlyingArchive();
	int32 Count = Values.Num();
	auto Array = Parent.EnterArray(Name, Count);
	if (Archive.IsLoading()) Values.SetNum(Count);
	for (T& Value : Values)
	{
		auto Element = Array.EnterElement().EnterRecord();
		SerializeData(Element, Value);
	}
}

template <typename T>
void UAnimGraph::SerializeEnum(FStructuredArchiveRecord& Parent, const char* Name, T& Value, T LastValue)
{
	FArchive& Archive = Parent.GetUnderlyingArchive();
	int32 Number = Archive.IsLoading() ? 0 : static_cast<int32>(Value);
	Parent << SA_VALUE(Name, Number);
	if (Number < 0 || Number > static_cast<int32>(LastValue))
	{
		Archive.SetError();
		return;
	}
	if (Archive.IsLoading()) Value = static_cast<T>(Number);
}

template <typename T>
T* UAnimGraph::BuildAnimGraph(TMap<int32, FAnimNode_Base*>& NodeInstances, int32 TargetID)
{
	if (NodeInstances.Contains(TargetID))
	{
		return static_cast<T*>(NodeInstances[TargetID]);
	}

	auto It = std::find_if(Nodes.begin(), Nodes.end(), [TargetID](const FAnimNodeData& Node) { return Node.NodeID == TargetID; });
	if (It == Nodes.end())
	{
		return nullptr;
	}

	const FAnimNodeData& NodeData = *It;

	FAnimNode_Base* Node = nullptr;
	switch (NodeData.Type)
	{
		case EAnimNodeDataType::AnimSequencePlayer:
		{
			const FAnimSequencePlayerData& ActualData = AnimSequencePlayers[NodeData.ActualDataIndex];

			FAnimNode_SequencePlayer* Player = new FAnimNode_SequencePlayer();
			Player->Sequence = UAssetManager::GetAssetByPath<UAnimSequence>(ActualData.SequencePath);
			Player->bLooping = ActualData.bLooping;
			Player->Speed = ActualData.Speed;

			Node = Player;
			break;
		}
		case EAnimNodeDataType::AnimTwoWayBlend:
		{
			const FAnimTwoWayBlendData& ActualData = AnimTwoWayBlends[NodeData.ActualDataIndex];

			FAnimNode_TwoWayBlend* TwoWayBlend = new FAnimNode_TwoWayBlend();
			TwoWayBlend->A = BuildAnimGraph<FAnimNode_Base>(NodeInstances, ActualData.NodeIDA);
			TwoWayBlend->B = BuildAnimGraph<FAnimNode_Base>(NodeInstances, ActualData.NodeIDB);
			TwoWayBlend->Alpha = ActualData.Alpha;

			Node = TwoWayBlend;
			break;
		}
		case EAnimNodeDataType::BlendSpacePlayer:
		{
			const FBlendSpacePlayerData& ActualData = BlendSpacePlayers[NodeData.ActualDataIndex];

			FAnimNode_BlendSpacePlayer* Player = new FAnimNode_BlendSpacePlayer();
			Player->SetBlendSpace(UAssetManager::GetAssetByPath<UBlendSpace>(ActualData.BlendSpacePath));
			Player->SetX(ActualData.X);
			Player->SetY(ActualData.Y);
			Player->bLooping = ActualData.bLooping;
			Player->Speed = ActualData.Speed;

			Node = Player;
			break;
		}
		case EAnimNodeDataType::AnimStateMachine:
		{
			const FAnimStateMachineData& ActualData = StateMachines[NodeData.ActualDataIndex];

			FAnimNode_StateMachine* StateMachine = new FAnimNode_StateMachine();

			for (const FAnimStateData& StateData : ActualData.States)
			{
				FAnimNode_Base* Node = BuildAnimGraph<FAnimNode_Base>(NodeInstances, StateData.NodeID);
				StateMachine->States.Add(Node);
			}

			auto FindStateIndexFromStateID = [ActualData](int32 StateID) -> int32 {
				auto It = std::find_if(ActualData.States.begin(), ActualData.States.end(), [StateID](const FAnimStateData& StateData) { return StateData.StateID == StateID; });
				if (It != ActualData.States.end())
				{
					return std::distance(ActualData.States.begin(), It);
				}
				return -1;
			};

			for (const FAnimTransitionData& TransitionData : ActualData.AnyTransitions)
			{
				FAnimStateTransition& Transition = StateMachine->AnyTransitions.AddDefaulted_GetRef();
				Transition.CrossfadeDuration = TransitionData.CrossfadeDuration;
				Transition.NextState = FindStateIndexFromStateID(TransitionData.NextStateID);
				Transition.Condition.ParameterName = FName(TransitionData.Condition.ParameterName);
				Transition.Condition.ComparisonOperator = TransitionData.Condition.ComparisonOperator;
				Transition.Condition.Value = TransitionData.Condition.Value;
			}

			StateMachine->Transitions.SetNum(StateMachine->States.Num());
			for (const FAnimTransitionData& TransitionData : ActualData.Transitions)
			{
				int32 StateIndex = FindStateIndexFromStateID(TransitionData.PreviousStateID);

				FAnimStateTransition& Transition = StateMachine->Transitions[StateIndex].AddDefaulted_GetRef();
				Transition.CrossfadeDuration = TransitionData.CrossfadeDuration;
				Transition.NextState = FindStateIndexFromStateID(TransitionData.NextStateID);
				Transition.Condition.ParameterName = FName(TransitionData.Condition.ParameterName);
				Transition.Condition.ComparisonOperator = TransitionData.Condition.ComparisonOperator;
				Transition.Condition.Value = TransitionData.Condition.Value;
			}

			StateMachine->SetEntryState(FindStateIndexFromStateID(ActualData.InitStateID));

			Node = StateMachine;

			break;
		}
	}

	if (!Node)
	{
		return nullptr;
	}

	NodeInstances[TargetID] = Node;

	return Node;
}

void UAnimGraph::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	Record << SA_VALUE("RootNodeID", RootNodeID);
	SerializeArray(Record, "Nodes", Nodes);
	// Parameter types must be loaded before transition union values.
	SerializeArray(Record, "Parameters", Parameters);
	SerializeArray(Record, "AnimSequencePlayers", AnimSequencePlayers);
	SerializeArray(Record, "AnimTwoWayBlends", AnimTwoWayBlends);
	SerializeArray(Record, "BlendSpacePlayers", BlendSpacePlayers);
	SerializeArray(Record, "StateMachines", StateMachines);
}

UAnimInstance* UAnimGraph::CreateAnimInstance()
{
	UAnimInstance* Instance = NewObject<UAnimInstance>();

	TMap<int32, FAnimNode_Base*> NodeInstances;
	FAnimNode_Base* RootNodeData = BuildAnimGraph<FAnimNode_Base>(NodeInstances, RootNodeID);

	Instance->SetRoot(RootNodeData);

	for (const FAnimParameterData& ParamData : Parameters)
	{
		switch (ParamData.Type)
		{
			case EAnimParameterType::Int32: Instance->AddParameter(ParamData.Name, ParamData.Value.Int32); break;
			case EAnimParameterType::Float: Instance->AddParameter(ParamData.Name, ParamData.Value.Float); break;
			case EAnimParameterType::Bool: Instance->AddParameter(ParamData.Name, ParamData.Value.Bool); break;
		}
	}

	return Instance;
}

void UAnimGraph::SerializeValue(FStructuredArchiveRecord& Parent, EAnimParameterType Type, FAnimParameterValue& Value)
{
	switch (Type)
	{
		case EAnimParameterType::Int32: Parent << SA_VALUE("Value", Value.Int32); break;
		case EAnimParameterType::Float: Parent << SA_VALUE("Value", Value.Float); break;
		case EAnimParameterType::Bool: Parent << SA_VALUE("Value", Value.Bool); break;
		default: Parent.GetUnderlyingArchive().SetError(); break;
	}
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FBlendSpacePlayerData& Player)
{
	Record << SA_VALUE("BlendSpacePath", Player.BlendSpacePath);
	Record << SA_VALUE("Looping", Player.bLooping);
	Record << SA_VALUE("Speed", Player.Speed);
	Record << SA_VALUE("X", Player.X);
	Record << SA_VALUE("Y", Player.Y);
}

const FAnimParameterData* UAnimGraph::FindParameterData(const FString& Name) const
{
	for (const FAnimParameterData& Parameter : Parameters)
	{
		if (Parameter.Name == Name)
		{
			return &Parameter;
		}
	}
	return nullptr;
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimTransitionConditionData& Condition)
{
	Record << SA_VALUE("ParameterName", Condition.ParameterName);
	SerializeEnum(Record, "ComparisonOperator", Condition.ComparisonOperator, EAnimComparisonOperator::LessEqual);
	const FAnimParameterData* Parameter = FindParameterData(Condition.ParameterName);
	if (!Parameter)
	{
		HTR_LOG(Error, "AnimGraph: transition references undefined parameter '{}'.", Condition.ParameterName);
		Record.GetUnderlyingArchive().SetError();
		return;
	}
	SerializeValue(Record, Parameter->Type, Condition.Value);
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimTransitionData& Transition)
{
	Record << SA_VALUE("PreviousStateID", Transition.PreviousStateID);
	Record << SA_VALUE("NextStateID", Transition.NextStateID);
	Record << SA_VALUE("CrossfadeDuration", Transition.CrossfadeDuration);
	auto ConditionRecord = Record.EnterRecord("Condition");
	SerializeData(ConditionRecord, Transition.Condition);
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimStateMachineData& Machine)
{
	Record << SA_VALUE("InitStateID", Machine.InitStateID);
	SerializeArray(Record, "States", Machine.States);
	SerializeArray(Record, "AnyTransitions", Machine.AnyTransitions);
	SerializeArray(Record, "Transitions", Machine.Transitions);
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimNodeData& Node)
{
	Record << SA_VALUE("NodeID", Node.NodeID);
	SerializeEnum(Record, "Type", Node.Type, EAnimNodeDataType::AnimStateMachine);
	Record << SA_VALUE("ActualType", Node.ActualDataIndex);
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimParameterData& Parameter)
{
	Record << SA_VALUE("Name", Parameter.Name);
	SerializeEnum(Record, "Type", Parameter.Type, EAnimParameterType::Float);
	SerializeValue(Record, Parameter.Type, Parameter.Value);
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimSequencePlayerData& Player)
{
	Record << SA_VALUE("SequencePath", Player.SequencePath);
	Record << SA_VALUE("Looping", Player.bLooping);
	Record << SA_VALUE("Speed", Player.Speed);
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimTwoWayBlendData& Blend)
{
	Record << SA_VALUE("NodeIDA", Blend.NodeIDA);
	Record << SA_VALUE("NodeIDB", Blend.NodeIDB);
	Record << SA_VALUE("Alpha", Blend.Alpha);
}

void UAnimGraph::SerializeData(FStructuredArchiveRecord& Record, FAnimStateData& State)
{
	Record << SA_VALUE("StateID", State.StateID);
	Record << SA_VALUE("Name", State.Name);
	Record << SA_VALUE("NodeID", State.NodeID);
}

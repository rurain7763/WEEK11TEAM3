#pragma once

#include "Core/EngineString.h"
#include "Containers/Array.h"
#include "Animation/StateMachine/AnimStateTransition.h"

enum class EAnimNodeDataType
{
	AnimSequencePlayer,
	AnimTwoWayBlend,
	BlendSpacePlayer,
	AnimStateMachine
};

struct FAnimNodeData
{
	int32 NodeID;
	EAnimNodeDataType Type;
	int32 ActualDataIndex;
};

struct FAnimSequencePlayerData
{
	FString SequencePath;
	bool bLooping;
	float Speed;
};

struct FAnimTwoWayBlendData
{
	int32 NodeIDA;
	int32 NodeIDB;
	float Alpha;
};

struct FBlendSpacePlayerData
{
	FString BlendSpacePath;
	bool bLooping;
	float Speed;
	float X;
	float Y;
};

struct FAnimStateData
{
	int32 StateID;
	FString Name;
	int32 NodeID;
};

struct FAnimTransitionConditionData
{
	FString ParameterName;
	EAnimComparisonOperator ComparisonOperator;
	FAnimParameterValue Value;
};

struct FAnimTransitionData
{
	int32 PreviousStateID = -1;
	int32 NextStateID = -1;
	float CrossfadeDuration = 0.f;
	FAnimTransitionConditionData Condition;
	bool bHasExitTime = false;
	float ExitTime = 1.f;
};

struct FAnimStateMachineData
{
	int32 InitStateID;
	FString Name;
	TArray<FAnimStateData> States;
	TArray<FAnimTransitionData> AnyTransitions;
	TArray<FAnimTransitionData> Transitions;
};

struct FAnimParameterData
{
	FString Name;
	EAnimParameterType Type;
	FAnimParameterValue Value;
};

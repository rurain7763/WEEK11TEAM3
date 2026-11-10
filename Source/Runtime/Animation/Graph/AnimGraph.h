#pragma once

#include "Asset/Asset.h"
#include "Animation/Graph/AnimGraphData.h"
#include "Containers/Map.h"

class UAnimInstance;
struct FAnimNode_Base;

class UAnimGraph : public UAsset
{
	DECLARE_CLASS(UAnimGraph, UAsset)

public:
	virtual void Serialize(FStructuredArchive::FRecord Record) override;

	inline void SetRootNodeID(int32 InRootNodeID)
	{
		RootNodeID = InRootNodeID;
	}

	inline int32 GetRootNodeID() const
	{
		return RootNodeID;
	}

	inline TArray<FAnimNodeData>& GetNodes()
	{
		return Nodes;
	}

	inline TArray<FAnimSequencePlayerData>& GetAnimSequencePlayers()
	{
		return AnimSequencePlayers;
	}

	inline TArray<FAnimTwoWayBlendData>& GetAnimTwoWayBlends()
	{
		return AnimTwoWayBlends;
	}

	inline TArray<FBlendSpacePlayerData>& GetBlendSpacePlayers()
	{
		return BlendSpacePlayers;
	}

	inline TArray<FAnimStateMachineData>& GetAnimStateMachines()
	{
		return StateMachines;
	}

	inline TArray<FAnimParameterData>& GetAnimParameters()
	{
		return Parameters;
	}

	UAnimInstance* CreateAnimInstance();

private:
	template <typename T>
	void SerializeArray(FStructuredArchiveRecord& Parent, const char* Name, TArray<T>& Values);

	template <typename T>
	void SerializeEnum(FStructuredArchiveRecord& Parent, const char* Name, T& Value, T LastValue);

	void SerializeValue(FStructuredArchiveRecord& Parent, EAnimParameterType Type, FAnimParameterValue& Value);

	void SerializeData(FStructuredArchiveRecord& Record, FAnimNodeData& Node);

	void SerializeData(FStructuredArchiveRecord& Record, FAnimParameterData& Parameter);

	void SerializeData(FStructuredArchiveRecord& Record, FAnimSequencePlayerData& Player);

	void SerializeData(FStructuredArchiveRecord& Record, FAnimTwoWayBlendData& Blend);

	void SerializeData(FStructuredArchiveRecord& Record, FBlendSpacePlayerData& Player);

	void SerializeData(FStructuredArchiveRecord& Record, FAnimStateData& State);

	const FAnimParameterData* FindParameterData(const FString& Name) const;

	void SerializeData(FStructuredArchiveRecord& Record, FAnimTransitionConditionData& Condition);

	void SerializeData(FStructuredArchiveRecord& Record, FAnimTransitionData& Transition);

	void SerializeData(FStructuredArchiveRecord& Record, FAnimStateMachineData& Machine);

	template <typename T>
	T* BuildAnimGraph(TMap<int32, FAnimNode_Base*>& NodeInstances, int32 TargetID);

private:
	int32 RootNodeID;
	TArray<FAnimNodeData> Nodes;
	TArray<FAnimSequencePlayerData> AnimSequencePlayers;
	TArray<FAnimTwoWayBlendData> AnimTwoWayBlends;
	TArray<FBlendSpacePlayerData> BlendSpacePlayers;
	TArray<FAnimStateMachineData> StateMachines;
	TArray<FAnimParameterData> Parameters;
};

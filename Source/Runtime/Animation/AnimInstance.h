#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Animation/AnimNotify.h"
#include "Animation/AnimParameter.h"
#include "Animation/AnimationTypes.h"
#include "Core/Delegate.h"
#include "Graph/AnimGraphData.h"

struct FAnimNode_Base;
struct FAnimNode_StateMachine;

class UAnimInstance : public UObject
{
	DECLARE_CLASS(UAnimInstance, UObject)

public:
	virtual ~UAnimInstance() = default;

	void SetRoot(FAnimNode_Base* InRoot)
	{
		RootNode = InRoot;
	}

	void AddStateMachine(const FName& InName, FAnimNode_StateMachine* InStateMachine) {
		StateMachines.Add(InName, InStateMachine);
	}

	virtual void NativeInitializeAnimation();

	virtual void NativeUpdateAnimation(float DeltaTime);

	virtual void Evaluate(FPoseContext& OutPoseContext);

	void DispatchQueuedAnimEvents();

	bool HasRootMotion() const
	{
		return bHasRootMotion;
	}

	bool PlayState(const FName& InMachineName, const FName& InStateName, float CrossfadeDuration);

	FTransform ConsumeRootMotion();

	template <typename T>
	inline void AddParameter(const FName& Name, T InitValue)
	{
		ParameterTable.AddParameter<T>(Name, InitValue);
	}

	inline void AddTrigger(const FName& Name)
	{
		ParameterTable.AddTrigger(Name);
	}

	inline void SetTrigger(const FName& Name)
	{
		ParameterTable.SetTrigger(Name);
	}

	inline void ResetTrigger(const FName& Name)
	{
		ParameterTable.ResetTrigger(Name);
	}

	inline void RemoveParameter(const FName& Name)
	{
		ParameterTable.RemoveParameter(Name);
	}

	inline bool HasParameter(const FName& Name)
	{
		return ParameterTable.HasParameter(Name);
	}

protected:
	TMulticastDelegate<const FAnimNotifyEvent&> OnFireNotify;

	FAnimNode_Base* RootNode = nullptr;
	TMap<FName, FAnimNode_StateMachine*, FNameHash> StateMachines;
	FAnimationUpdateContext UpdateContext;
	TArray<FAnimNotifyEvent*> EventQueue;
	FAnimParameterTable ParameterTable;
	bool bHasRootMotion;
	FTransform RootMotion;
};

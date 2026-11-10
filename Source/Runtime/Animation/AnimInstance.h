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
	friend struct FAnimGraphActionInputTest; // Temporary test-only access; remove with the input harness.

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

	FTransform ConsumeRootMotion();

	template <typename T>
	inline void AddParameter(const FName& Name, T InitValue)
	{
		ParameterTable.AddParameter<T>(Name, InitValue);
	}

	inline void RemoveParameter(const FName& Name)
	{
		ParameterTable.RemoveParameter(Name);
	}

	inline void SetFloat(const FName& Name, float InValue) {
		ParameterTable.SetFloat(Name, InValue);
	}
	inline void SetBool(const FName& Name, bool InValue) {
		ParameterTable.SetBool(Name, InValue);
	}
	inline void SetInt(const FName& Name, int32 InValue) {
		ParameterTable.SetInt(Name, InValue);
	}

	inline float GetFloat(const FName& Name) const {
		float Value = 0.f;
		ParameterTable.TryGetFloat(Name, Value);
		return Value;
	}
	inline bool GetBool(const FName& Name) const {
		bool Value = false;
		ParameterTable.TryGetBool(Name, Value);
		return Value;
	}
	inline int32 GetInt(const FName& Name) const {
		int32 Value = 0;
		ParameterTable.TryGetInt(Name, Value);
		return Value;
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

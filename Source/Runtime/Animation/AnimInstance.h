#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Animation/AnimNotify.h"
#include "Animation/AnimParameter.h"
#include "Animation/AnimationTypes.h"
#include "Core/Delegate.h"

struct FAnimNode_Base;

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

protected:
	TMulticastDelegate<const FAnimNotifyEvent&> OnFireNotify;

	FAnimNode_Base* RootNode = nullptr;
	FAnimationUpdateContext UpdateContext;
	TArray<FAnimNotifyEvent*> EventQueue;
	FAnimParameterTable ParameterTable;
	bool bHasRootMotion;
	FTransform RootMotion;
};

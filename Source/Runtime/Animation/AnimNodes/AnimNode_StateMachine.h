#pragma once

#include "Animation/AnimNodes/AnimNode_Base.h"
#include "Animation/StateMachine/AnimStateTransition.h"
#include "Containers/Array.h"

struct FAnimNode_StateMachine : public FAnimNode_Base
{
	TArray<FAnimNode_Base*> States;
	TArray<FAnimStateTransition> AnyTransitions;
	TArray<TArray<FAnimStateTransition>> Transitions;

	int32 CurrentStateIndex = -1;
	bool bActiveTransition = false;
	FAnimActiveTransition ActiveTransition;

	void SetEntryState(int32 StateIndex)
	{
		CurrentStateIndex = StateIndex;
	}

	void Initialize() override;

	void Update(FAnimationUpdateContext& UpdateContext) override;

	FAnimStateTransition* EvaluateTransitions(const TArray<FAnimStateTransition>& Transitions, const FAnimParameterTable& ParameterTable, bool& OutIsTriggerTransition) const;

	void Evaluate(FPoseContext& OutPoseContext) override;

	virtual float GetNormalizedTime() const override;
};

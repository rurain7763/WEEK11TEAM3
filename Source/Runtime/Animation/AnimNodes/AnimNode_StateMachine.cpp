#include "EnginePCH.h"
#include "Animation/AnimNodes/AnimNode_StateMachine.h"
#include "Animation/AnimationRuntime.h"

void FAnimNode_StateMachine::Initialize()
{
	if (CurrentStateIndex == -1)
	{
		return;
	}

	States[CurrentStateIndex]->Initialize();
}

void FAnimNode_StateMachine::Update(FAnimationUpdateContext& UpdateContext)
{
	if (!bActiveTransition)
	{
		FAnimStateTransition* Transition = Transition = EvaluateTransitions(AnyTransitions, *UpdateContext.ParameterTable);
		if (!Transition && CurrentStateIndex != -1)
		{
			Transition = EvaluateTransitions(Transitions[CurrentStateIndex], *UpdateContext.ParameterTable);
		}

		if (Transition)
		{
			if (Transition->CrossfadeDuration > 0.f)
			{
				bActiveTransition = true;
				ActiveTransition.CrossfadeDuration = Transition->CrossfadeDuration;
				ActiveTransition.ElapsedTime = 0.f;
				ActiveTransition.PreviousState = CurrentStateIndex;
				ActiveTransition.NextState = Transition->NextState;
			}
			else
			{
				CurrentStateIndex = Transition->NextState;
			}

			States[Transition->NextState]->Initialize();
		}
	}

	if (bActiveTransition)
	{
		if (ActiveTransition.ElapsedTime > ActiveTransition.CrossfadeDuration)
		{
			bActiveTransition = false;
			CurrentStateIndex = ActiveTransition.NextState;

			States[CurrentStateIndex]->Update(UpdateContext);
		}
		else
		{
			FAnimNode_Base* PrevState = States[ActiveTransition.PreviousState];
			FAnimNode_Base* NextState = States[ActiveTransition.NextState];

			FAnimationUpdateContext ContextPrev = UpdateContext;
			ContextPrev.bHasRootMotion = false;
			ContextPrev.RootMotion = FTransform::Identity;
			PrevState->Update(ContextPrev);

			FAnimationUpdateContext ContextNext = UpdateContext;
			ContextNext.bHasRootMotion = false;
			ContextNext.RootMotion = FTransform::Identity;
			NextState->Update(ContextNext);

			ActiveTransition.ElapsedTime += UpdateContext.DeltaTime;

			float Alpha = FMath::Clamp(ActiveTransition.ElapsedTime / ActiveTransition.CrossfadeDuration, 0.f, 1.f);

			if (Alpha == 0.f)
			{
				UpdateContext.bHasRootMotion |= ContextPrev.bHasRootMotion;
			}
			else if (Alpha == 1.f)
			{
				UpdateContext.bHasRootMotion |= ContextNext.bHasRootMotion;
			}
			else
			{
				UpdateContext.bHasRootMotion |= ContextPrev.bHasRootMotion | ContextNext.bHasRootMotion;
			}

			UpdateContext.RootMotion = FTransform::Blend(ContextPrev.RootMotion, ContextNext.RootMotion, Alpha);
		}
	}
	else
	{
		if (CurrentStateIndex == -1)
		{
			return;
		}

		States[CurrentStateIndex]->Update(UpdateContext);
	}
}

FAnimStateTransition* FAnimNode_StateMachine::EvaluateTransitions(const TArray<FAnimStateTransition>& Transitions, const FAnimParameterTable& ParameterTable) const
{
	for (int32 i = 0; i < Transitions.Num(); i++)
	{
		const FAnimStateTransition& Transition = Transitions[i];
		if (Transition.NextState == CurrentStateIndex)
		{
			continue;
		}

		if (!ParameterTable.HasParameter(Transition.Condition.ParameterName))
		{
			continue;
		}

		const FAnimParameter& Param = ParameterTable.FindParameter(Transition.Condition.ParameterName);
		if (!Transition.Condition.Evaluate(Param))
		{
			continue;
		}

		return const_cast<FAnimStateTransition*>(&Transitions[i]);
	}

	return nullptr;
}

void FAnimNode_StateMachine::Evaluate(FPoseContext& OutPoseContext)
{
	if (bActiveTransition)
	{
		// TODO: Do tansition
		FAnimNode_Base* PrevState = States[ActiveTransition.PreviousState];
		FAnimNode_Base* NextState = States[ActiveTransition.NextState];

		FPoseContext PrevPose;
		PrevState->Evaluate(PrevPose);

		FPoseContext NextPose;
		NextState->Evaluate(NextPose);

		float Alpha = FMath::Clamp(ActiveTransition.ElapsedTime / ActiveTransition.CrossfadeDuration, 0.f, 1.f);
		FAnimationRuntime::BlendTwoPosesTogether(PrevPose, NextPose, Alpha, OutPoseContext);
	}
	else
	{
		if (CurrentStateIndex == -1)
		{
			return;
		}

		States[CurrentStateIndex]->Evaluate(OutPoseContext);
	}
}

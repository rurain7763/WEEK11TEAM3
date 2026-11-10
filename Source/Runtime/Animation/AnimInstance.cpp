#include "EnginePCH.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNodes/AnimNode_Base.h"

void UAnimInstance::NativeInitializeAnimation()
{
	if (!RootNode)
	{
		return;
	}

	RootNode->Initialize();

	UpdateContext.ParameterTable = &ParameterTable;
	UpdateContext.EventQueue = &EventQueue;
}

void UAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	if (!RootNode)
	{
		return;
	}

	UpdateContext.bHasRootMotion = false;
	UpdateContext.RootMotion = FTransform::Identity;
	UpdateContext.DeltaTime = DeltaTime;

	RootNode->Update(UpdateContext);

	bHasRootMotion = UpdateContext.bHasRootMotion;
	RootMotion = UpdateContext.RootMotion;
}

void UAnimInstance::Evaluate(FPoseContext& OutPoseContext)
{
	if (!RootNode)
	{
		return;
	}

	RootNode->Evaluate(OutPoseContext);
}

void UAnimInstance::DispatchQueuedAnimEvents()
{
	for (const FAnimNotifyEvent* Notify : EventQueue)
	{
		OnFireNotify.Broadcast(*Notify);
	}
	EventQueue.Reset();
}

FTransform UAnimInstance::ConsumeRootMotion()
{
	FTransform Temp = RootMotion;
	bHasRootMotion = false;
	RootMotion = FTransform::Identity;
	return Temp;
}

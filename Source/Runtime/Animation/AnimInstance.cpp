#include "EnginePCH.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNodes/AnimNode_Base.h"
#include "Animation/AnimNodes/AnimNode_StateMachine.h"
#include "Component/SkeletalMeshComponent.h"

void UAnimInstance::NativeInitializeAnimation()
{
	if (!RootNode)
	{
		return;
	}

	RootNode->Initialize();

	UpdateContext.ParameterTable = &ParameterTable;
	UpdateContext.EventQueue = &EventQueue;
	CurrentActiveEventVersion = 0;
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
	UpdateContext.bExtractRootMotion = !MeshComp || MeshComp->ShouldExtractRootMotion();

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

float UAnimInstance::GetNormalizedTime() const
{
	return RootNode ? RootNode->GetNormalizedTime() : -1.f;
}

void UAnimInstance::DispatchQueuedAnimEvents()
{
	for (const FAnimNotifyEventReference& EventRef : EventQueue)
	{
		FAnimNotifyEvent* Notify = EventRef.Notify;

		if (Notify->Duration <= 0.f)
		{
			OnFireNotify.Broadcast(*Notify);
			continue;
		}

		UAnimNotifyState* NotifyState = Notify->NotifyState;
		if (!NotifyState)
		{
			continue;
		}
		
		FAnimNotifyEventKey NotifyKey;
		NotifyKey.SourcePlayerID = EventRef.SourcePlayerID;
		NotifyKey.NotifyPtr = (uintptr_t) Notify;

		if (FActiveAnimNotifyEventEntry* Existing = ActiveEvents.Find(NotifyKey))
		{
			NotifyState->NotifyTick(MeshComp, UpdateContext.DeltaTime, EventRef);
			if (EventRef.bReachedEnd)
			{
				NotifyState->NotifyEnd(MeshComp, EventRef.Sequence, *Notify);
				ActiveEvents.Remove(NotifyKey);
			}
			else
			{
				Existing->Version = CurrentActiveEventVersion;
			}
		}
		else
		{
			NotifyState->NotifyBegin(MeshComp, EventRef.Sequence, *Notify);
			NotifyState->NotifyTick(MeshComp, UpdateContext.DeltaTime, EventRef);
			if (EventRef.bReachedEnd)
			{
				NotifyState->NotifyEnd(MeshComp, EventRef.Sequence, *Notify);
			}
			else
			{
				FActiveAnimNotifyEventEntry NewEntry;
				NewEntry.Sequence = EventRef.Sequence;
				NewEntry.Copied = *Notify;
				NewEntry.Version = CurrentActiveEventVersion;

				ActiveEvents.Add(NotifyKey, NewEntry);
			}
		}
	}

	for (auto Itr = ActiveEvents.begin(); Itr != ActiveEvents.end();)
	{
		FActiveAnimNotifyEventEntry& Entry = Itr->second;
		if (Entry.Version == CurrentActiveEventVersion)
		{
			Itr++;
			continue;
		}

		Entry.Copied.NotifyState->NotifyEnd(MeshComp, Entry.Sequence, Entry.Copied);
		Itr = ActiveEvents.Remove(Itr);
	}

	EventQueue.Reset();
	CurrentActiveEventVersion++;
}

bool UAnimInstance::PlayState(const FName& InMachineName, const FName& InStateName, float CrossfadeDuration)
{
	FAnimNode_StateMachine **Found = StateMachines.Find(InMachineName);
	if (Found == nullptr) return false;

	return (*Found)->RequestState(InStateName, CrossfadeDuration);
}

FTransform UAnimInstance::ConsumeRootMotion()
{
	FTransform Temp = RootMotion;
	bHasRootMotion = false;
	RootMotion = FTransform::Identity;
	return Temp;
}

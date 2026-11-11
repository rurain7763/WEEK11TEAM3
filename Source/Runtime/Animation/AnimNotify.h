#pragma once

#include "Core/NameTypes.h"
#include "UObject/Object.h"
#include "Containers/Array.h"

class UAnimInstance;
class UAnimSequenceBase;
class USkeletalMeshComponent;

struct FAnimNotifyEvent;

class UAnimNotifyState : public UObject
{
	DECLARE_CLASS(UAnimNotifyState, UObject)

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Sequence, const FAnimNotifyEvent& Event) {}
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Sequence, float DeltaTime, const FAnimNotifyEvent& Event) {}
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Sequence, const FAnimNotifyEvent& Event) {}
};

struct FAnimNotifyEvent
{
	FName Name;
	float TriggerTime = 0.f;
	float Duration = 0.f;
	UAnimNotifyState* NotifyState = nullptr;

	inline float GetEndTriggerTime()
	{
		return TriggerTime + Duration;
	}
};

struct FAnimNotifyEventReference
{
	UAnimSequenceBase* Sequence;
	int32 SourcePlayerID;
	FAnimNotifyEvent* Notify;
	bool bReachedEnd;
};

struct FAnimNotifyContext
{
	int32 SourcePlayerID;
	TArray<FAnimNotifyEventReference> CollectedNotifies;
};

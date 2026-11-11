#pragma once

#include "Core/NameTypes.h"
#include "UObject/Object.h"
#include "Containers/Array.h"

class UAnimNotifyState : public UObject
{
public:
	virtual void NotifyBegin() {}
	virtual void NotifyTick() {}
	virtual void NotifyEnd() {}
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
	int32 SourcePlayerID;
	FAnimNotifyEvent* Notify;
	bool bReachedEnd;
};

struct FAnimNotifyContext
{
	int32 SourcePlayerID;
	TArray<FAnimNotifyEventReference> CollectedNotifies;
};

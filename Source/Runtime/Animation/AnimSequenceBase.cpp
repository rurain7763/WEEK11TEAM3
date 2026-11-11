#include "EnginePCH.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimDataModel.h"
#include <algorithm>

void UAnimSequenceBase::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	FArchive& Ar = Record.GetUnderlyingArchive();

	int32 NotifyNum = Notifies.Num();
	auto NotifyArray = Record.EnterArray("Notifies", NotifyNum);
	if (Ar.IsLoading())
	{
		Notifies.SetNum(NotifyNum);
	}

	for (int32 i = 0; i < NotifyNum; i++)
	{
		FAnimNotifyEvent& Notify = Notifies[i];

		auto ElementRecord = NotifyArray.EnterElement().EnterRecord();

		ElementRecord << SA_VALUE("Name", Notify.Name);
		ElementRecord << SA_VALUE("TriggerTime", Notify.TriggerTime);
		ElementRecord << SA_VALUE("Duration", Notify.Duration);
		
		FString ClssName;
		if (Notify.NotifyState)
		{
			UClass* Clss = Notify.NotifyState->GetClass();
			ClssName = Clss->Name;
		}
		ElementRecord << SA_VALUE("NotifyStateName", ClssName);
	
		if (Ar.IsLoading())
		{
			UClass* Clss = FindClass(ClssName);
			if (Clss && Clss->IsChildOf(UAnimNotifyState::StaticClass()))
			{
				Notify.NotifyState = NewObject<UAnimNotifyState>(this, Clss);
			}
		}

		if (Notify.NotifyState)
		{
			Notify.NotifyState->Serialize(ElementRecord);
		}
	}
}

float UAnimSequenceBase::GetDuration() const
{
	if (DataModel)
	{
		return DataModel->GetDuration();
	}

	return 0.f;
}

void UAnimSequenceBase::GetAnimNotifies(float Duration, bool bLooping, float StartTime, float DeltaTime, FAnimNotifyContext& NotifyContext)
{
	bool bIncludeStart = false;
	while (DeltaTime > 0.f)
	{
		float NextTime = StartTime + DeltaTime;

		float DeltaStep = 0.f;
		if (NextTime > Duration)
		{
			GetAnimNotifiesFromPositions(StartTime, Duration, bIncludeStart, NotifyContext);

			if (bLooping)
			{
				DeltaStep = Duration - StartTime;
				StartTime = 0.f;
				bIncludeStart = true;
			}
			else
			{
				DeltaStep = DeltaTime;
			}
		}
		else
		{
			GetAnimNotifiesFromPositions(StartTime, NextTime, bIncludeStart, NotifyContext);
			DeltaStep = DeltaTime;
		}

		DeltaTime -= DeltaStep;
	}
}

void UAnimSequenceBase::GetAnimNotifiesFromPositions(float PreviousPosition, float CurrentPosition, bool bIncludeStart, FAnimNotifyContext& NotifyContext)
{
	for (int32 i = 0; i < Notifies.Num(); i++)
	{
		FAnimNotifyEvent& Notify = Notifies[i];

		const float StartTriggerTime = Notify.TriggerTime;
		const float EndTriggerTime = Notify.GetEndTriggerTime();

		if (StartTriggerTime > CurrentPosition)
		{
			break;
		}

		// NOTE: 지금 구간에 겹치는 이벤트만을 수집한다.
		if (StartTriggerTime <= CurrentPosition && (bIncludeStart ? EndTriggerTime >= PreviousPosition : EndTriggerTime > PreviousPosition))
		{
			FAnimNotifyEventReference& Ref = NotifyContext.CollectedNotifies.AddDefaulted_GetRef();
			Ref.Sequence = this;
			Ref.SourcePlayerID = NotifyContext.SourcePlayerID;
			Ref.Notify = &Notify;
			Ref.bReachedEnd = EndTriggerTime <= CurrentPosition;
		}
	}
}

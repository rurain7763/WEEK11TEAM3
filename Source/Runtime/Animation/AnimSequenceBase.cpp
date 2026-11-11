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
	const bool bReverse = DeltaTime < 0.f;
	const float Boundary = bReverse ? 0.f : Duration; // 경계값

	if (Duration <= 0.f)
	{
		return;
	}

	bool bIncludeStart = false;

	if (!bReverse)
	{
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
	else
	{
		while (DeltaTime < 0.f)
		{
			float NextTime = StartTime + DeltaTime;

			const bool bCrossedBoundary =  NextTime < Boundary; // 0초보다 작은지
			const float EndTime = bCrossedBoundary ? Boundary : NextTime;

			if (EndTime < StartTime)
			{
				GetAnimNotifiesFromPositions(StartTime, EndTime, bIncludeStart, NotifyContext);
			}

			if (!bCrossedBoundary || !bLooping)
			{
				break;
			}

			DeltaTime -= EndTime - StartTime;
			StartTime = Duration;
			bIncludeStart = true;
		}
	}
}

void UAnimSequenceBase::GetAnimNotifiesFromPositions(float PreviousPosition, float CurrentPosition, bool bIncludeStart, FAnimNotifyContext& NotifyContext)
{
	if (CurrentPosition < PreviousPosition)
	{
		for (int32 i = Notifies.Num() - 1; i >= 0; --i)
		{
			FAnimNotifyEvent& Notify = Notifies[i];
			const float TriggerTime = Notify.TriggerTime;

			if (TriggerTime < CurrentPosition)
			{
				break;
			}

			const bool bPassedStart = TriggerTime < PreviousPosition || (bIncludeStart && TriggerTime == PreviousPosition);

			if (Notify.Duration == 0.f && bPassedStart)
			{
				auto& Ref = NotifyContext.CollectedNotifies.AddDefaulted_GetRef();
				Ref.SourcePlayerID = NotifyContext.SourcePlayerID;
				Ref.Notify = &Notify;
				Ref.bReachedEnd = true;
			}
		}

		return;
	}

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
			Ref.SourcePlayerID = NotifyContext.SourcePlayerID;
			Ref.Notify = &Notify;
			Ref.bReachedEnd = EndTriggerTime <= CurrentPosition;
		}
	}
}

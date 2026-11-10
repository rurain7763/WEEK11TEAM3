#include "EnginePCH.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimDataModel.h"
#include <algorithm>

float UAnimSequenceBase::GetDuration() const
{
	if (DataModel)
	{
		return DataModel->GetDuration();
	}

	return 0.f;
}

void UAnimSequenceBase::GetAnimNotifies(float Duration, bool bLooping, float StartTime, float DeltaTime, TArray<FAnimNotifyEvent*>& OutEvents)
{
	bool bIncludeStart = false;
	while (DeltaTime > 0.f)
	{
		float NextTime = StartTime + DeltaTime;

		float DeltaStep = 0.f;
		if (NextTime > Duration)
		{
			GetAnimNotifiesFromPositions(StartTime, Duration, bIncludeStart, OutEvents);

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
			GetAnimNotifiesFromPositions(StartTime, NextTime, bIncludeStart, OutEvents);
			DeltaStep = DeltaTime;
		}

		DeltaTime -= DeltaStep;
	}
}

void UAnimSequenceBase::GetAnimNotifiesFromPositions(float PreviousPosition, float CurrentPosition, bool bIncludeStart, TArray<FAnimNotifyEvent*>& OutEvents)
{
	auto Lower = bIncludeStart ? std::lower_bound(Notifies.begin(), Notifies.end(), PreviousPosition, [](const FAnimNotifyEvent& Element, float Pos) { return Element.TriggerTime < Pos; }) : std::upper_bound(Notifies.begin(), Notifies.end(), PreviousPosition, [](float Pos, const FAnimNotifyEvent& Element) { return Pos < Element.TriggerTime; });
	auto Upper = std::upper_bound(Notifies.begin(), Notifies.end(), CurrentPosition, [](float Pos, const FAnimNotifyEvent& Element) { return Pos < Element.TriggerTime; });

	for (auto Itr = Lower; Itr != Upper; Itr++)
	{
		OutEvents.Emplace(&(*Itr));
	}
}

#include "EnginePCH.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"

UAnimSingleNodeInstance::UAnimSingleNodeInstance()
{
	Player = new FAnimNode_SequencePlayer();
	RootNode = Player;
}

UAnimSingleNodeInstance::~UAnimSingleNodeInstance()
{
	delete Player;
}

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAsset* InAsset)
{
	if (UAnimSequence* Sequence = Cast<UAnimSequence>(InAsset))
	{
		CurrentAsset = InAsset;
		Player->Sequence = Sequence;
	}
}

#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimNodes/AnimNode_SequencePlayer.h"

class UAnimationAsset;

class UAnimSingleNodeInstance : public UAnimInstance
{
	DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

public:
	UAnimSingleNodeInstance();

	virtual ~UAnimSingleNodeInstance();

	void SetAnimationAsset(UAnimationAsset* InAsset);

	void SetLoop(bool InbLoop)
	{
		Player->bLooping = InbLoop;
	}

private:
	UAnimationAsset* CurrentAsset = nullptr;
	FAnimNode_SequencePlayer* Player = nullptr;
};

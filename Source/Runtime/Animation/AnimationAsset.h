#pragma once

#include "Asset/Asset.h"

class USkeleton;

class UAnimationAsset : public UAsset
{
	DECLARE_CLASS(UAnimationAsset, UAsset)

public:
	virtual ~UAnimationAsset() = default;

	virtual void Serialize(FStructuredArchive::FRecord Record);

	void SetSkeleton(USkeleton* InSkeleton)
	{
		Skeleton = InSkeleton;
	}

	virtual float GetDuration() const
	{
		return 0.f;
	}

	USkeleton* GetSkeleton() const
	{
		return Skeleton;
	}

private:
	USkeleton* Skeleton;
};

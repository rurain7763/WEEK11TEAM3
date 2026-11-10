#pragma once

#include "Math/EngineMath.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"

struct FTransform
{
	FTransform() = default;
	FTransform(const FVector& InL, const FVector& InS, const FRotator& InR);
	FTransform(const FVector& InL, const FVector& InS, const FQuat& InR);

	inline FTransform operator*(const FTransform& Other) const
	{
		FVector CombinedScale = Other.Scale * Scale;
		FQuat CombinedRot = Other.GetOrientation() * GetOrientation();
		FVector CombinedLocation = Other.Rotation * (Other.Scale * Location) + Other.Location;

		return FTransform(CombinedLocation, CombinedScale, CombinedRot);
	}

	inline FTransform& operator*=(const FTransform& Other)
	{
		FVector CombinedScale = Other.Scale * Scale;
		FQuat CombinedRot = Other.GetOrientation() * GetOrientation();
		FVector CombinedLocation = Other.Rotation * (Other.Scale * Location) + Other.Location;

		Location = CombinedLocation;
		Rotation = CombinedRot.ToFRotator();
		Scale = CombinedScale;

		return *this;
	}

	static FTransform Blend(const FTransform& A, const FTransform& B, float T);

	static FTransform Identity;

    FMatrix GetLocalMatrix() const;

	FTransform GetRelativeTransform(const FTransform& Other) const;

	FVector GetForward() const;
	FVector GetUp() const;
	FVector GetRight() const;
	FQuat GetOrientation() const;

	// Decomposes an affine TRS matrix without shear. A collapsed axis yields identity rotation.
	void SetFromMatrix(const FMatrix& InMatrix);

	FVector Location = FVector(0, 0, 0);
	FVector Scale = FVector(1, 1, 1);
	FRotator Rotation = FRotator(0, 0, 0);
};


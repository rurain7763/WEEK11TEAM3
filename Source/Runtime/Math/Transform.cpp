#include "EnginePCH.h"
#include "Transform.h"

FTransform::FTransform(const FVector& InL, const FVector& InS, const FRotator& InR)
{
	Location = InL;
	Scale = InS;
	Rotation = InR;
}

FTransform::FTransform(const FVector& InL, const FVector& InS, const FQuat& InR)
    : Location(InL)
    , Scale(InS)
    , Rotation(InR.ToFRotator())
{
}

FTransform FTransform::Blend(const FTransform& A, const FTransform& B, float T)
{
    return FTransform(
        FMath::Lerp(A.Location, B.Location, T),
        FMath::Lerp(A.Scale, B.Scale, T),
        FQuat::Slerp(A.GetOrientation(), B.GetOrientation(), T)
    );
}

FMatrix FTransform::GetLocalMatrix() const
{
    return FMatrix::MakeFromTRS(Location, Rotation.Quaternion(), Scale);
}

FTransform FTransform::GetRelativeTransform(const FTransform& Other) const
{
    FTransform Result = FTransform::Identity;
    Result.SetFromMatrix(GetLocalMatrix() * Other.GetLocalMatrix().Inverse());
    return Result;
}

FVector FTransform::GetForward() const
{
    FQuat Quat = GetOrientation();
    return Quat.RotateVector(FVector(1.0f, 0.0f, 0.0f));
}

FVector FTransform::GetUp() const
{
    FQuat Quat = GetOrientation();
    return Quat.RotateVector(FVector(0.0f, 0.0f, 1.0f));
}

FVector FTransform::GetRight() const
{
    return GetOrientation().RotateVector(FVector(0.0f, 1.0f, 0.0f));
}

FQuat FTransform::GetOrientation() const
{
    return FQuat::MakeFromEuler(FMath::DegreesToRadians(Rotation.Roll), FMath::DegreesToRadians(Rotation.Pitch), FMath::DegreesToRadians(Rotation.Yaw));
}

void FTransform::SetFromMatrix(const FMatrix& InMatrix)
{
    FQuat Orientation;
    InMatrix.Decompose(Location, Orientation, Scale);
    Rotation = Orientation.ToFRotator();
}

FTransform FTransform::Identity = FTransform(FVector(0, 0, 0), FVector(1, 1, 1), FRotator(0, 0, 0));

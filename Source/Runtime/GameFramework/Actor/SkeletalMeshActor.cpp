#include "EnginePCH.h"
#include "SkeletalMeshActor.h"

#include "Component/PrimitiveComponent.h"




ASkeletalMeshActor::ASkeletalMeshActor()
{
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("USkeletalMeshComponent");
	SkeletalMeshComponent->SetupAttachment(GetRootComponent());
}

void ASkeletalMeshActor::SetPrimitiveType(EPrimitiveType Type)
{

}

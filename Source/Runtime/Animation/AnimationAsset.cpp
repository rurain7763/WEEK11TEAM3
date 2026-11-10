#include "EnginePCH.h"
#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "Asset/AssetManager.h"

void UAnimationAsset::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	FArchive& Archive = Record.GetUnderlyingArchive();

	FString SkeletonPath = Skeleton ? Skeleton->GetPath() : "";
	Record << SA_VALUE("Skeleton", SkeletonPath);
	if (Archive.IsLoading())
	{
		Skeleton = UAssetManager::GetAssetByPath<USkeleton>(SkeletonPath);
	}
}

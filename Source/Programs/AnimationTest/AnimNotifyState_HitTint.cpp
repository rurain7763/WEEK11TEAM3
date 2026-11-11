#include "EnginePCH.h"

#include "Animation/AnimNotify.h"
#include "Component/SkeletalMeshComponent.h"
#include "Containers/Map.h"
#include "Render/Material.h"

class UAnimNotifyState_HitTint : public UAnimNotifyState
{
	DECLARE_CLASS(UAnimNotifyState_HitTint, UAnimNotifyState)

public:
	void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase*, const FAnimNotifyEvent&) override
	{
		if (!MeshComp)
		{
			return;
		}

		FActiveTint& Active = ActiveTints[MeshComp->GetUUID()];
		if (Active.Count++ > 0)
		{
			return;
		}

		for (int32 Slot = 0; Slot < MeshComp->GetNumMaterials(); ++Slot)
		{
			TUniquePtr<UMaterial> Tinted(UMaterial::CreateInstance(MeshComp->GetMaterial(Slot)));
			if (!Tinted)
			{
				continue;
			}

			Tinted->BaseColor = FLinearColor(1.f, 0.f, 0.f, Tinted->BaseColor.A);
			UMaterial* OriginalOverride = MeshComp->GetOverrideMaterial(Slot);
			MeshComp->SetMaterial(Slot, Tinted.get());
			Active.Slots.Add({ Slot, OriginalOverride, std::move(Tinted) });
		}
	}

	void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase*, const FAnimNotifyEvent&) override
	{
		if (!MeshComp)
		{
			return;
		}

		const uint32 MeshID = MeshComp->GetUUID();
		FActiveTint* Active = ActiveTints.Find(MeshID);
		if (!Active || --Active->Count > 0)
		{
			return;
		}

		for (const FTintedSlot& Slot : Active->Slots)
		{
			if (MeshComp->GetOverrideMaterial(Slot.Index) == Slot.Material.get())
			{
				MeshComp->SetMaterial(Slot.Index, Slot.OriginalOverride);
			}
		}
		ActiveTints.Remove(MeshID);
	}

private:
	struct FTintedSlot
	{
		int32 Index;
		UMaterial* OriginalOverride;
		TUniquePtr<UMaterial> Material;
	};

	struct FActiveTint
	{
		int32 Count = 0;
		TArray<FTintedSlot> Slots;
	};

	TMap<uint32, FActiveTint> ActiveTints;
};

#pragma once

#include "GameFramework/Actor.h"
#include "Component/BillboardComponent.h"
#include "Component/DecalComponent.h"

class AFakeSpotLightActor : public AActor
{
	DECLARE_CLASS(AFakeSpotLightActor, AActor)

public:
	AFakeSpotLightActor();
	virtual ~AFakeSpotLightActor() override = default;

	virtual void Tick(float DeltaTime) override;
	using Super::Serialize;
	void Serialize(FStructuredArchive::FRecord Record) override;

protected:
	FString RemapLoadedComponentName(const FString& Name) const override;

private:
	void ResolveComponents();

	UDecalComponent* DecalComponent = nullptr;
	UBillboardComponent* LightBeamBillboardComponent = nullptr;
	UBillboardComponent* IconBillboardComponent = nullptr;
};

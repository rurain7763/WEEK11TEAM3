#include "EnginePCH.h"
#include "FakeSpotLightActor.h"
#include "Core/ResourcePaths.h"
#include "Asset/AssetManager.h"
#include <algorithm>
#include <cmath>

AFakeSpotLightActor::AFakeSpotLightActor()
{
	USceneComponent* PreviousRoot = GetRootComponent();
	DecalComponent = CreateDefaultSubobject<UDecalComponent>(UDecalComponent::StaticClass()->GetName());
	SetRootComponent(DecalComponent);
	DestroyComponent(PreviousRoot);

	if (UMaterial* DefaultMaterial = DecalComponent->GetDecalMaterial())
	{
		UMaterial* Material = UMaterial::CreateInstance(DefaultMaterial);
		Material->Textures[0] = UAssetManager::Get().LoadTexture(ResourcePaths::FakeSpotLightImage);
		DecalComponent->SetDecalMaterial(Material);
	}

	LightBeamBillboardComponent = CreateDefaultSubobject<UBillboardComponent>("LightBeamBillboard");
	LightBeamBillboardComponent->SetupAttachment(DecalComponent);
	LightBeamBillboardComponent->SetHiddenInDetails(false);
	LightBeamBillboardComponent->SetTexture(ResourcePaths::FakeSpotLightBeamImage);
	LightBeamBillboardComponent->SetHiddenInGame(false);
	LightBeamBillboardComponent->SetAligned(EBillboardAlignment::AxisAligned);
	LightBeamBillboardComponent->SetAxisLock(EBillboardAxisLock::NegX);

	IconBillboardComponent = CreateDefaultSubobject<UBillboardComponent>("IconBillboard");
	IconBillboardComponent->SetupAttachment(DecalComponent);
	IconBillboardComponent->SetHiddenInDetails(true);
	IconBillboardComponent->SetTexture(ResourcePaths::FakeSpotLightIcon);

	PrimaryActorTick.bCanEverTick = true;
	bTickInEditor = true;
}

FString AFakeSpotLightActor::RemapLoadedComponentName(const FString& Name) const
{
	if (Name == "LightVolumeBillboard") return "LightBeamBillboard";
	return Super::RemapLoadedComponentName(Name);
}

void AFakeSpotLightActor::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);
	if (Record.GetUnderlyingArchive().IsLoading())
	{
		ResolveComponents();
	}
}

void AFakeSpotLightActor::ResolveComponents()
{
	// 로드·PIE 복제로 루트가 교체될 수 있으므로 생성자에서 저장한 주소를 재사용하지 않는다.
	DecalComponent = Cast<UDecalComponent>(GetRootComponent());
	if (!DecalComponent)
		DecalComponent = Cast<UDecalComponent>(FindComponentByClass(UDecalComponent::StaticClass()));

	LightBeamBillboardComponent = Cast<UBillboardComponent>(FindComponentByName(FName("LightBeamBillboard")));
	UBillboardComponent* LegacyBeam = Cast<UBillboardComponent>(FindComponentByName(FName("LightVolumeBillboard")));
	if (LegacyBeam && LightBeamBillboardComponent && LegacyBeam != LightBeamBillboardComponent)
	{
		// 이전 이름으로 로드된 빔은 이 액터에서만 이관한다.
		// AActor의 일반 컴포넌트 조회에는 파생 클래스의 이름 규칙을 넣지 않는다.
		LightBeamBillboardComponent->SetMaterial(0, LegacyBeam->GetMaterial(0));
		DestroyComponent(LegacyBeam);
	}
	if (!LightBeamBillboardComponent)
		LightBeamBillboardComponent = LegacyBeam;
	IconBillboardComponent = Cast<UBillboardComponent>(FindComponentByName(FName("IconBillboard")));

	if (!DecalComponent)
		return;
	if (LightBeamBillboardComponent)
	{
		LightBeamBillboardComponent->SetupAttachment(DecalComponent);
		LightBeamBillboardComponent->SetAligned(EBillboardAlignment::AxisAligned);
		LightBeamBillboardComponent->SetAxisLock(EBillboardAxisLock::NegX);
		LightBeamBillboardComponent->SetHiddenInGame(false);
	}
	if (IconBillboardComponent)
		IconBillboardComponent->SetupAttachment(DecalComponent);
}

void AFakeSpotLightActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!DecalComponent || !LightBeamBillboardComponent)
		return;

	const FVector Size = DecalComponent->GetDecalSize();
	const FVector ParentScale = DecalComponent->GetWorldScale3D();
	// 빔은 Decal 중심에서 +X 면까지 뻗는다. Size.X는 박스 전체 길이다.
	const float BeamLength = Size.X * std::fabs(ParentScale.X) * 0.5f;
	const float BeamWidth = std::max(Size.Y * std::fabs(ParentScale.Y), Size.Z * std::fabs(ParentScale.Z));

	// 평면 중심을 빔 길이의 절반만큼 옮기면 텍스처 위쪽 끝이 Decal 중심에 놓인다.
	LightBeamBillboardComponent->SetRelativeLocation(FVector(Size.X * 0.25f, 0.f, 0.f));
	LightBeamBillboardComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));

	constexpr float MinScale = 1.0e-6f;
	const float RelativeWidth = std::fabs(ParentScale.Y) > MinScale ? BeamWidth / ParentScale.Y : 0.0f;
	const float RelativeHeight = std::fabs(ParentScale.Z) > MinScale ? BeamLength / ParentScale.Z : 0.0f;
	LightBeamBillboardComponent->SetRelativeScale3D(FVector(1.f, RelativeWidth, RelativeHeight));
}

#pragma once

#include "Actor.h"
#include "LightComponents.h"
#include "UTextComponent.h"

class ASpotLight : public AActor
{
	REFLECT_CLASS(ASpotLight, AActor)

public:
	ASpotLight()
	{
		USpotLightComponent* SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(FName("SpotLightComponent"));
		SetRootComponent(SpotLightComponent);
	}

	void CreateEditorComponents() override
	{
		UBillboardComponent* BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(FName("SpotLightIcon"));
		BillboardComponent->SetTexture(FAssetManager::Get().GetAssetAs<FTexture2DAsset>(BuiltInAssetID::SpotLightIcon, true));
		BillboardComponent->SetBlendState(ERenderBlendMode::Transparent);
		BillboardComponent->SetDepthState(true, false);
		BillboardComponent->SetEditorOnly(true);
		BillboardComponent->SetDoNotSerialize(true);
		BillboardComponent->SetVisualizeProxy(true);

		USceneComponent* RootComp = GetRootComponent();
		if (RootComp)
		{
			BillboardComponent->SetupAttachment(RootComp, false);
		}

		AddOwnedComponent(BillboardComponent);

		UText3DComponent* Text3DComponent = CreateDefaultSubobject<UText3DComponent>(FName("UUIDDisplayer"));
		Text3DComponent->SetRelativeScale3D(FVector(0.01f, 0.01f, 0.01f));
		Text3DComponent->SetBillboard(true);
		Text3DComponent->SetText(Utf2Wide(std::format("UUID: {}", UUID)));
		Text3DComponent->SetFontAtlasAsset(FAssetManager::Get().GetAssetAs<FFontAtlasAsset>(FName("TestFontAtlas")));
		Text3DComponent->SetDepthState(false, false);
		Text3DComponent->SetEditorOnly(true);
		Text3DComponent->SetDoNotSerialize(true);

		Text3DComponent->SetupAttachment(BillboardComponent, false);

		AddOwnedComponent(Text3DComponent);
	}
};
#pragma once

#include "Actor.h"
#include "FAssetManager.h"
#include "UBillboardComponent.h"
#include "UHeightFogComponent.h"
#include "UTextComponent.h"

class AHeightFog : public AActor
{
	REFLECT_CLASS(AHeightFog, AActor)

public:
	AHeightFog()
	{
		UHeightFogComponent* HeightFogComponent = CreateDefaultSubobject<UHeightFogComponent>(FName("HeightFogComponent"));
		SetRootComponent(HeightFogComponent);
	}

	void CreateEditorComponents() override
	{
		UBillboardComponent* BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(FName("HeightFogIcon"));
		BillboardComponent->SetTexture(FAssetManager::Get().GetAssetAs<FTexture2DAsset>(BuiltInAssetID::HeightFogIcon, true));
		BillboardComponent->SetBlendState(ERenderBlendMode::Transparent);
		BillboardComponent->SetDepthState(true, false);
		BillboardComponent->SetEditorOnly(true);
		BillboardComponent->SetDoNotSerialize(true);
		BillboardComponent->SetVisualizeProxy(true);

		USceneComponent* RootComp = GetRootComponent();
		if (RootComp)
		{
			BillboardComponent->SetupAttachment(RootComp);
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

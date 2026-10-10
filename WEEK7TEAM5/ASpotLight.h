#pragma once

#include "Actor.h"
#include "FAssetManager.h"
#include "UBillboardComponent.h"
#include "UTextComponent.h"
#include "USpotLightComponent.h"

class ASpotLight : public AActor
{
	REFLECT_CLASS(ASpotLight, AActor)

public:
	// 에디터 아이콘은 USpotLightComponent가, UUID 표시는 AActor::CreateEditorComponents가 만든다.
	ASpotLight()
	{
		USpotLightComponent* SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(FName("SpotLightComponent"));
		SetRootComponent(SpotLightComponent);
	}
};

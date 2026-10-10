#pragma once

#include "ULightComponent.h"

class UDirectionalLightComponent final : public ULightComponent
{
	REFLECT_CLASS(UDirectionalLightComponent, ULightComponent)

public:
	UDirectionalLightComponent();
	FVector GetDirection();
};


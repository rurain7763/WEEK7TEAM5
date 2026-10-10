#pragma once

#include "ULightComponent.h"

class UPointLightComponent : public ULightComponent
{
	REFLECT_CLASS(UPointLightComponent, ULightComponent)

public:
	UPointLightComponent();
};


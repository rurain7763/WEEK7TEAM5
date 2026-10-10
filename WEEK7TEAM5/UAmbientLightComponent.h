#pragma once

#include "ULightComponent.h"

class UAmbientLightComponent final : public ULightComponent
{
	REFLECT_CLASS(UAmbientLightComponent, ULightComponent)

public:
	UAmbientLightComponent();
};


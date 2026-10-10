#pragma once

#include "ULightComponent.h"

class UPointLightComponent : public ULightComponent
{
	REFLECT_CLASS(UPointLightComponent, ULightComponent)

public:
	UPointLightComponent();

    float GetAttenuationRadius() const;
    float GetRadiusFallOffExponent() const;

    void SetAttenuationRadius(float Radius);
    void SetLightFallOffExponent(float FallOff);

private:
    float mAttenuationRadius;
    float mLightFallOffExponent;
};
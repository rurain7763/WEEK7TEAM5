
#include "UPointLightComponent.h"

UPointLightComponent::UPointLightComponent()
	: Super(ELightType::Point)
	, mAttenuationRadius(0.5f)
	, mLightFallOffExponent(8.f)
{
}

float UPointLightComponent::GetAttenuationRadius() const
{
    return mAttenuationRadius;
}

float UPointLightComponent::GetRadiusFallOffExponent() const
{
    return mLightFallOffExponent;
}

void UPointLightComponent::SetAttenuationRadius(float Radius)
{
    assert(Radius >= 0.f);

    mAttenuationRadius = Radius;
}

void UPointLightComponent::SetLightFallOffExponent(float FallOff)
{
    assert(FallOff >= 0.001f);

    mLightFallOffExponent = FallOff;
}
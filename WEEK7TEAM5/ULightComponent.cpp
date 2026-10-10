#include "ULightComponent.h"

ULightComponent::ULightComponent()
    : mLightType(ELightType::None)
{
}

ULightComponent::ULightComponent(ELightType LightType)
    : mLightType(LightType)
{
}

ELightType ULightComponent::GetLightType() const
{
    return mLightType;
}
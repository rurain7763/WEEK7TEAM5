#include "USpotLightComponent.h"

USpotLightComponent::USpotLightComponent()
	: Super(ELightType::Spot)
	, mRange(5.f)
	, mLightFallOffExponent(8.f)
	, mColor(FVector4(1.f, 1.f, 1.f, 1.f))
	, mInnerConeAngle(30.f)
	, mOuterConeAngle(45.f)
{
	SetTickable(true);
}

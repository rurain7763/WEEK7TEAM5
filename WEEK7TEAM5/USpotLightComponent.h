#pragma once

#include "ULightComponent.h"


// Todo: Need to refactor
class USpotLightComponent : public ULightComponent
{
	REFLECT_CLASS(USpotLightComponent, ULightComponent)

public:
	USpotLightComponent();

	/*
	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		Ar << Range;
		Ar << mInnerConeAngle;
		Ar << mOuterConeAngle;
		Ar << mColor;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << Range;
		Ar << mInnerConeAngle;
		Ar << mOuterConeAngle;
		Ar << mColor;
	}
	*/

	inline float GetRange() const;
	inline float GetInnerConeAngle() const;
	inline float GetOuterConeAngle() const;
	inline const FVector4& GetColor() const;

	inline void SetRange(float Range);
	inline void SetColor(const FVector4& Color);
	inline void SetOuterConeAngle(float Angle);
	inline void SetInnerConeAngle(float Angle);

	inline float GetRadiusFallOffExponent() const;
	inline void SetLightFallOffExponent(float FallOff);

private:
	static constexpr float MAX_CONE_ANGLE = 89.f;

	float mRange;
	FVector4 mColor;
	float mInnerConeAngle;
	float mOuterConeAngle;

	// Todo: Code duplicate
	float mLightFallOffExponent;
};

inline float USpotLightComponent::GetRange() const
{ 
	return mRange; 
}

inline float USpotLightComponent::GetInnerConeAngle() const
{ 
	return mInnerConeAngle; 
}

inline float USpotLightComponent::GetOuterConeAngle() const
{ 
	return mOuterConeAngle; 
}

inline const FVector4& USpotLightComponent::GetColor() const
{ 
	return mColor; 
}

inline float USpotLightComponent::GetRadiusFallOffExponent() const
{
	return mLightFallOffExponent;
}

inline void USpotLightComponent::SetLightFallOffExponent(float FallOff)
{
	assert(FallOff >= 0.001f);

	mLightFallOffExponent = FallOff;
}

inline void USpotLightComponent::SetRange(float Range)
{
	assert(Range >= 0, f);

	mRange = Range;
}

inline void USpotLightComponent::SetColor(const FVector4& Color) 
{ 
	mColor = Color; 
}

inline void USpotLightComponent::SetOuterConeAngle(float Angle)
{
	mOuterConeAngle = FMath::Clamp(Angle, 0.f, MAX_CONE_ANGLE);
	mInnerConeAngle = FMath::Min(mInnerConeAngle, mOuterConeAngle);
}

inline void USpotLightComponent::SetInnerConeAngle(float Angle)
{
	mInnerConeAngle = FMath::Clamp(Angle, 0.f, mOuterConeAngle);
}

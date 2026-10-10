#include "ULightComponentBase.h"

ULightComponentBase::ULightComponentBase()
	: mIntensity(1.f)
	, mLightColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
	, mbVisible(true)
{
}

float ULightComponentBase::GetIntensity() const
{
	return mIntensity;
}

FLinearColor ULightComponentBase::GetLightColor() const
{
	return mLightColor;
}

bool ULightComponentBase::IsVisible() const
{
	return mbVisible;
}

void ULightComponentBase::SetIntensity(float Intensity)
{
	assert(Intensity >= 0.0f);

	mIntensity = Intensity;
}

void ULightComponentBase::SetLightColor(const FLinearColor& Color)
{
	mLightColor = Color;
}

void ULightComponentBase::SetVisible(bool bVisible)
{
	mbVisible = bVisible;
}
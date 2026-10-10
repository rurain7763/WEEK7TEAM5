#include "USpotLightComponent.h"

#include "FArchive.h"
#include "Serializers.h"
#include "Assets.h"

USpotLightComponent::USpotLightComponent()
{
	Intensity = 1.f;
	SetTickable(true);
}

// Intensity와 Color는 ULightComponentBase가 직렬화한다.
void USpotLightComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << mInnerConeAngle;
	Ar << mOuterConeAngle;
}

void USpotLightComponent::Deserialize(FArchive& Ar)
{
	Super::Deserialize(Ar);

	Ar << mInnerConeAngle;
	Ar << mOuterConeAngle;
}

void USpotLightComponent::SerializeClass(json::JSON& outJson) const
{
	Super::SerializeClass(outJson);

	outJson["Properties"]["InnerConeAngle"] = mInnerConeAngle;
	outJson["Properties"]["OuterConeAngle"] = mOuterConeAngle;
}

void USpotLightComponent::DeserializeClass(const json::JSON& inJson)
{
	Super::DeserializeClass(inJson);

	const json::JSON& PropertiesJson = inJson.at("Properties");

	if (PropertiesJson.hasKey("InnerConeAngle"))
	{
		mInnerConeAngle = JsonUtils::FromJson<float>(PropertiesJson.at("InnerConeAngle"));
	}

	if (PropertiesJson.hasKey("OuterConeAngle"))
	{
		mOuterConeAngle = JsonUtils::FromJson<float>(PropertiesJson.at("OuterConeAngle"));
	}
}

void USpotLightComponent::SetOuterConeAngle(float InAngle)
{
	mOuterConeAngle = FMath::Clamp(InAngle, 0.f, MAX_CONE_ANGLE);
	mInnerConeAngle = FMath::Min(mInnerConeAngle, mOuterConeAngle);
}

void USpotLightComponent::SetInnerConeAngle(float InAngle)
{
	mInnerConeAngle = FMath::Clamp(InAngle, 0.f, mOuterConeAngle);
}

const FGuid& USpotLightComponent::GetEditorIconTextureID() const
{
	return BuiltInAssetID::SpotLightIcon;
}

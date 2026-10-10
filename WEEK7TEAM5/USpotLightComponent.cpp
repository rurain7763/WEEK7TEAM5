#include "USpotLightComponent.h"

#include "FArchive.h"
#include "Serializers.h"
#include "Assets.h"

USpotLightComponent::USpotLightComponent()
{
	Intensity = 1.f;
}

// Intensity와 Color는 ULightComponentBase가 직렬화한다.
void USpotLightComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << Range;
	Ar << mInnerConeAngle;
	Ar << mOuterConeAngle;
}

void USpotLightComponent::Deserialize(FArchive& Ar)
{
	Super::Deserialize(Ar);

	Ar << Range;
	Ar << mInnerConeAngle;
	Ar << mOuterConeAngle;
}

void USpotLightComponent::SerializeClass(json::JSON& outJson) const
{
	Super::SerializeClass(outJson);

	outJson["Properties"]["Range"] = Range;
	outJson["Properties"]["InnerConeAngle"] = mInnerConeAngle;
	outJson["Properties"]["OuterConeAngle"] = mOuterConeAngle;
}

void USpotLightComponent::DeserializeClass(const json::JSON& inJson)
{
	Super::DeserializeClass(inJson);

	const json::JSON& PropertiesJson = inJson.at("Properties");
	if (PropertiesJson.hasKey("Range"))
	{
		Range = JsonUtils::FromJson<float>(PropertiesJson.at("Range"));
	}
	// Outer를 먼저 넣어야 Inner가 Outer 기준으로 잘리지 않는다.
	if (PropertiesJson.hasKey("OuterConeAngle"))
	{
		SetOuterConeAngle(JsonUtils::FromJson<float>(PropertiesJson.at("OuterConeAngle")));
	}
	if (PropertiesJson.hasKey("InnerConeAngle"))
	{
		SetInnerConeAngle(JsonUtils::FromJson<float>(PropertiesJson.at("InnerConeAngle")));
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

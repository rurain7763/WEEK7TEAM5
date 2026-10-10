#include "UPointLightComponent.h"

#include "FArchive.h"
#include "Serializers.h"

UPointLightComponent::UPointLightComponent()
{
	Intensity = 1.f;
}

// Intensity와 Color는 ULightComponentBase가 직렬화한다.
void UPointLightComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << Radius;
	Ar << RadiusFallOff;
}

void UPointLightComponent::Deserialize(FArchive& Ar)
{
	Super::Deserialize(Ar);

	Ar << Radius;
	Ar << RadiusFallOff;
}

void UPointLightComponent::SerializeClass(json::JSON& outJson) const
{
	Super::SerializeClass(outJson);

	outJson["Properties"]["Radius"] = Radius;
	outJson["Properties"]["RadiusFallOff"] = RadiusFallOff;
}

void UPointLightComponent::DeserializeClass(const json::JSON& inJson)
{
	Super::DeserializeClass(inJson);

	const json::JSON& PropertiesJson = inJson.at("Properties");
	if (PropertiesJson.hasKey("Radius"))
	{
		Radius = JsonUtils::FromJson<float>(PropertiesJson.at("Radius"));
	}
	if (PropertiesJson.hasKey("RadiusFallOff"))
	{
		RadiusFallOff = JsonUtils::FromJson<float>(PropertiesJson.at("RadiusFallOff"));
	}
}

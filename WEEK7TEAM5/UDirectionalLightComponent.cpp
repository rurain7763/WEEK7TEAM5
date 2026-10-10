#include "UDirectionalLightComponent.h"

#include "FArchive.h"
#include "Serializers.h"

UDirectionalLightComponent::UDirectionalLightComponent()
{
	Intensity = 1.f;
}

void UDirectionalLightComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

void UDirectionalLightComponent::Deserialize(FArchive& Ar)
{
	Super::Deserialize(Ar);
}

void UDirectionalLightComponent::SerializeClass(json::JSON& outJson) const
{
	Super::SerializeClass(outJson);
}

void UDirectionalLightComponent::DeserializeClass(const json::JSON& inJson)
{
	Super::DeserializeClass(inJson);
}
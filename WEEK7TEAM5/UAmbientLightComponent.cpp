#include "UAmbientLightComponent.h"

#include "FArchive.h"
#include "Serializers.h"

UAmbientLightComponent::UAmbientLightComponent()
{
	Intensity = 0.1f;
}

void UAmbientLightComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

void UAmbientLightComponent::Deserialize(FArchive& Ar)
{
	Super::Deserialize(Ar);
}

void UAmbientLightComponent::SerializeClass(json::JSON& outJson) const
{
	Super::SerializeClass(outJson);
}

void UAmbientLightComponent::DeserializeClass(const json::JSON& inJson)
{
	Super::DeserializeClass(inJson);
}
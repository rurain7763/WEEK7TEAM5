#include "ULightComponentBase.h"

#include "FArchive.h"
#include "Serializers.h"
#include "Assets.h"
#include "UBillboardComponent.h"

void ULightComponentBase::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << Intensity;
	Ar << LightColor;
}

void ULightComponentBase::Deserialize(FArchive& Ar)
{
	Super::Deserialize(Ar);

	Ar << Intensity;
	Ar << LightColor;
}

void ULightComponentBase::SerializeClass(json::JSON& outJson) const
{
	Super::SerializeClass(outJson);

	outJson["Properties"]["Intensity"] = Intensity;
	outJson["Properties"]["Color"] = JsonUtils::ToJson(LightColor);
}

void ULightComponentBase::DeserializeClass(const json::JSON& inJson)
{
	Super::DeserializeClass(inJson);

	const json::JSON& PropertiesJson = inJson.at("Properties");
	if (PropertiesJson.hasKey("Intensity"))
	{
		Intensity = JsonUtils::FromJson<float>(PropertiesJson.at("Intensity"));
	}
	if (PropertiesJson.hasKey("Color"))
	{
		LightColor = JsonUtils::FromJson<FLinearColor>(PropertiesJson.at("Color"));
	}
}

void ULightComponentBase::SetIntensity(float InIntensity)
{
	Intensity = InIntensity;
}

float ULightComponentBase::GetIntensity() const
{
	return Intensity;
}

void ULightComponentBase::SetColor(const FLinearColor& InColor)
{
	LightColor = InColor;
	SyncEditorIconColor();
}

FLinearColor ULightComponentBase::GetColor() const
{
	return LightColor;
}

void ULightComponentBase::CreateEditorComponents()
{
	Super::CreateEditorComponents();

	CreateEditorIcon(GetEditorIconTextureID(), FName("LightIcon"));
	SyncEditorIconColor();
}

const FGuid& ULightComponentBase::GetEditorIconTextureID() const
{
	return BuiltInAssetID::PointLightIcon;
}

void ULightComponentBase::SyncEditorIconColor()
{
	if (UBillboardComponent* EditorIcon = GetEditorIcon())
	{
		EditorIcon->SetColor(LightColor.ToFVector4());
	}
}

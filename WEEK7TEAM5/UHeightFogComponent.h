#pragma once

#include "PrimitiveComponent.h"
#include "FEngine.h"
#include "GraphicsManager.h"
#include "FFogProcess.h"
#include "JsonUtil.h"
#include "FArchive.h"
#include "Serializers.h"

class UHeightFogComponent : public UPrimitiveComponent
{
	REFLECT_CLASS(UHeightFogComponent, UPrimitiveComponent)

public:
	UHeightFogComponent()
	{
		FogProcess = &GEngine->GetGraphicsManager().GetFogProcess();
		FogComponentCount++;
		FogProcess->SetEnabled(FogComponentCount > 0);
		FogProcess->RegisterFogComponent();
	}

	~UHeightFogComponent()
	{
		FogComponentCount--;
		FogProcess->ResetFogConstants();
		FogProcess->SetEnabled(FogComponentCount > 0);
		FogProcess->UnregisterFogComponent();
	}

	void CreateEditorComponents() override
	{
		Super::CreateEditorComponents();

		CreateEditorIcon(BuiltInAssetID::HeightFogIcon, FName("HeightFogIcon"));
	}

	inline void SetFogDensity(float Density) { FogProcess->FogConstants.FogDensity = Density; }
	inline float GetFogDensity() const { return FogProcess->FogConstants.FogDensity; }

	inline void SetFogHeightFalloff(float Falloff) { FogProcess->FogConstants.FogHeightFalloff = Falloff; }
	inline float GetFogHeightFalloff() const { return FogProcess->FogConstants.FogHeightFalloff; }

	inline void SetFogStartDistance(float StartDistance) { FogProcess->FogConstants.StartDistance = StartDistance; }
	inline float GetFogStartDistance() const { return FogProcess->FogConstants.StartDistance; }

	inline void SetFogCutoffDistance(float CutoffDistance) { FogProcess->FogConstants.FogCutoffDistance = CutoffDistance; }
	inline float GetFogCutoffDistance() const { return FogProcess->FogConstants.FogCutoffDistance; }

	inline void SetFogMaxOpacity(float MaxOpacity) { FogProcess->FogConstants.FogMaxOpacity = MaxOpacity; }
	inline float GetFogMaxOpacity() const { return FogProcess->FogConstants.FogMaxOpacity; }

	inline void SetFogHeight(float Height) { FogProcess->FogConstants.FogHeight = Height; }
	inline float GetFogHeight() const { return FogProcess->FogConstants.FogHeight; }

	inline void SetFogInscatteringColor(const FLinearColor& Color) { FogProcess->FogConstants.FogInscatteringColor = Color; }
	inline FLinearColor GetFogInscatteringColor() const { return FogProcess->FogConstants.FogInscatteringColor; }
	
	virtual void SerializeClass(json::JSON& OutJson) const override
	{
		Super::SerializeClass(OutJson);

		OutJson["Properties"]["FogDensity"] = GetFogDensity();
		OutJson["Properties"]["FogHeightFalloff"] = GetFogHeightFalloff();
		OutJson["Properties"]["FogStartDistance"] = GetFogStartDistance();
		OutJson["Properties"]["FogCutoffDistance"] = GetFogCutoffDistance();
		OutJson["Properties"]["FogMaxOpacity"] = GetFogMaxOpacity();
		OutJson["Properties"]["FogHeight"] = GetFogHeight();
		OutJson["Properties"]["FogInscatteringColor"] = JsonUtils::ToJson(GetFogInscatteringColor());

	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (PropertiesJson.hasKey("FogDensity"))
		{
			SetFogDensity(JsonUtils::FromJson<float>(PropertiesJson.at("FogDensity")));
		}
		if (PropertiesJson.hasKey("FogHeightFalloff"))
		{
			SetFogHeightFalloff(JsonUtils::FromJson<float>(PropertiesJson.at("FogHeightFalloff")));
		}
		if (PropertiesJson.hasKey("FogStartDistance"))
		{
			SetFogStartDistance(JsonUtils::FromJson<float>(PropertiesJson.at("FogStartDistance")));
		}
		if (PropertiesJson.hasKey("FogCutoffDistance"))
		{
			SetFogCutoffDistance(JsonUtils::FromJson<float>(PropertiesJson.at("FogCutoffDistance")));
		}
		if (PropertiesJson.hasKey("FogMaxOpacity"))
		{
			SetFogMaxOpacity(JsonUtils::FromJson<float>(PropertiesJson.at("FogMaxOpacity")));
		}
		if (PropertiesJson.hasKey("FogHeight"))
		{
			SetFogHeight(JsonUtils::FromJson<float>(PropertiesJson.at("FogHeight")));
		}
		if (PropertiesJson.hasKey("FogInscatteringColor"))
		{
			SetFogInscatteringColor(JsonUtils::FromJson<FLinearColor>(PropertiesJson.at("FogInscatteringColor")));
		}
	}

private:
	inline static int32 FogComponentCount = 0;

	FFogProcess* FogProcess;
};

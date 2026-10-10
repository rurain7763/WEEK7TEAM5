#pragma once

#include "Core.h"
#include "SceneComponent.h"
#include "JsonUtil.h"

class ULightComponentBase : public USceneComponent
{
	REFLECT_CLASS(ULightComponentBase, USceneComponent)

public:
	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		Ar << Intensity;
		Ar << LightColor;
		Ar << bVisible;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << Intensity;
		Ar << LightColor;
		Ar << bVisible;
	}

	virtual void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		outJson["Properties"]["Intensity"] = Intensity;
		outJson["Properties"]["LightColor"] = JsonUtils::ToJson(LightColor);
		outJson["Properties"]["bVisible"] = bVisible;
	}

	virtual void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (PropertiesJson.hasKey("Intensity"))
		{
			Intensity = JsonUtils::FromJson<float>(PropertiesJson.at("Intensity"));
		}

		if (PropertiesJson.hasKey("LightColor"))
		{
			LightColor = JsonUtils::FromJson<FLinearColor>(PropertiesJson.at("LightColor"));
		}

		if (PropertiesJson.hasKey("bVisible"))
		{
			bVisible = JsonUtils::FromJson<bool>(PropertiesJson.at("bVisible"));
		}
	}

	inline void SetIntensity(float InIntensity) { Intensity = InIntensity; }
	inline float GetIntensity() const { return Intensity; }

	inline void SetColor(const FLinearColor& InColor) { LightColor = InColor; }
	inline FLinearColor GetColor() const { return LightColor; }

protected:
	float Intensity = 1.0f;
	FLinearColor LightColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	bool bVisible = true;
};

class ULightComponent : public ULightComponentBase
{
	REFLECT_CLASS(ULightComponent, ULightComponentBase)

public:
	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);
	}

	virtual void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);
	}

	virtual void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);
	}
};

class UAmbientLightComponent : public ULightComponent
{
	REFLECT_CLASS(UAmbientLightComponent, ULightComponent)

public:
	UAmbientLightComponent()
	{
		Intensity = 0.1f;
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);
	}

	virtual void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);
	}

	virtual void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);
	}
};

class UDirectionalLightComponent : public ULightComponent
{
	REFLECT_CLASS(UDirectionalLightComponent, ULightComponent)

public:
	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);
	}

	virtual void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);
	}

	virtual void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);
	}

	inline FVector GetDirection() { return GetForwardVector(); }
};

class UPointLightComponent : public ULightComponent
{
	REFLECT_CLASS(UPointLightComponent, USceneComponent)

public:
	UPointLightComponent() = default;

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		Ar << Radius;
		Ar << RadiusFallOff;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << Radius;
		Ar << RadiusFallOff;
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		outJson["Properties"]["Radius"] = Radius;
		outJson["Properties"]["RadiusFallOff"] = RadiusFallOff;
	}

	void DeserializeClass(const json::JSON& inJson) override
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

	inline void SetRadius(float InRadius) { Radius = InRadius; }
	inline float GetRadius() const { return Radius; }

	inline void SetRadiusFallOff(float InRadiusFallOff) { RadiusFallOff = InRadiusFallOff; }
	inline float GetRadiusFallOff() const { return RadiusFallOff; }

private:
	float Radius = 5.f;
	float RadiusFallOff = 1.f;
};

class USpotLightComponent : public UPointLightComponent
{
	REFLECT_CLASS(USpotLightComponent, UPointLightComponent)

public:
	USpotLightComponent() { SetTickable(true); }

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		Ar << mInnerConeAngle;
		Ar << mOuterConeAngle;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << mInnerConeAngle;
		Ar << mOuterConeAngle;
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		outJson["Properties"]["InnerConeAngle"] = mInnerConeAngle;
		outJson["Properties"]["OuterConeAngle"] = mOuterConeAngle;
	}

	void DeserializeClass(const json::JSON& inJson) override
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

	inline float GetInnerConeAngle() const { return mInnerConeAngle; }
	inline float GetOuterConeAngle() const { return mOuterConeAngle; }

	inline void SetOuterConeAngle(float InAngle)
	{
		mOuterConeAngle = FMath::Clamp(InAngle, 0.f, MAX_CONE_ANGLE);
		mInnerConeAngle = FMath::Min(mInnerConeAngle, mOuterConeAngle);
	}

	inline void SetInnerConeAngle(float InAngle)
	{
		mInnerConeAngle = FMath::Clamp(InAngle, 0.f, mOuterConeAngle);
	}

private:
	static constexpr float MAX_CONE_ANGLE = PI * 0.5f; // 90 degrees in radians

	float mInnerConeAngle = FMath::DegreesToRadians(30.0f);
	float mOuterConeAngle = FMath::DegreesToRadians(45.0f);
};
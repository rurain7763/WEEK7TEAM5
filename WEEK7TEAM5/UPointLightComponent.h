#pragma once

#include "ULightComponentBase.h"
#include "Json/json.hpp"
#include "JsonUtil.h"
#include "Vector.h"

class FArchive;

class UPointLightComponent : public ULightComponentBase
{
	REFLECT_CLASS(UPointLightComponent, ULightComponentBase)

public:
	UPointLightComponent();

	void Serialize(FArchive& Ar) override;

	void Deserialize(FArchive& Ar) override;

	void SerializeClass(json::JSON& outJson) const override;

	void DeserializeClass(const json::JSON& inJson) override;

	inline void SetRadius(float InRadius) { Radius = InRadius; }
	inline float GetRadius() const { return Radius; }

	inline void SetRadiusFallOff(float InRadiusFallOff) { RadiusFallOff = InRadiusFallOff; }
	inline float GetRadiusFallOff() const { return RadiusFallOff; }

private:
	float Radius = 5.f;
	float RadiusFallOff = 1.f;
};

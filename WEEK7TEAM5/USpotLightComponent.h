#pragma once

#include "ULightComponentBase.h"
#include "Json/json.hpp"
#include "JsonUtil.h"
#include "Vector.h"

class FArchive;


class USpotLightComponent : public ULightComponentBase
{
	REFLECT_CLASS(USpotLightComponent, ULightComponentBase)

public:
	USpotLightComponent();

	void Serialize(FArchive& Ar) override;

	void Deserialize(FArchive& Ar) override;

	void SerializeClass(json::JSON& outJson) const override;

	void DeserializeClass(const json::JSON& inJson) override;

	inline float GetRange() const { return Range; }
	inline float GetInnerConeAngle() const { return mInnerConeAngle; }
	inline float GetOuterConeAngle() const { return mOuterConeAngle; }

	void SetOuterConeAngle(float InAngle);
	void SetInnerConeAngle(float InAngle);

protected:
	const FGuid& GetEditorIconTextureID() const override;

private:
	static constexpr float MAX_CONE_ANGLE = 89.f;

	float Range = 5.0f;
	float mInnerConeAngle = 30.0f;
	float mOuterConeAngle = 45.0f;
};

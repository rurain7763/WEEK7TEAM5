#pragma once

#include "UPointLightComponent.h"
#include "Json/json.hpp"
#include "JsonUtil.h"
#include "Vector.h"

class FArchive;


class USpotLightComponent : public UPointLightComponent
{
	REFLECT_CLASS(USpotLightComponent, UPointLightComponent)

public:
	USpotLightComponent();

	virtual void Serialize(FArchive& Ar) override;
	

	virtual void Deserialize(FArchive& Ar) override;
	

	void SerializeClass(json::JSON& outJson) const override;
	

	void DeserializeClass(const json::JSON& inJson) override;
	

	inline float GetInnerConeAngle() const { return mInnerConeAngle; }
	inline float GetOuterConeAngle() const { return mOuterConeAngle; }

	void SetOuterConeAngle(float InAngle);	

	void SetInnerConeAngle(float InAngle);	

protected:
	const FGuid& GetEditorIconTextureID() const override;

private:
	static constexpr float MAX_CONE_ANGLE = PI * 0.5f; // 90 degrees in radians

	float mInnerConeAngle = FMath::DegreesToRadians(30.0f);
	float mOuterConeAngle = FMath::DegreesToRadians(45.0f);
};

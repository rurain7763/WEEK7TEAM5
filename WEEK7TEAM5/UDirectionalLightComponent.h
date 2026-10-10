#pragma once

#include "ULightComponentBase.h"
#include "Json/json.hpp"
#include "JsonUtil.h"
#include "Vector.h"

class FArchive;


class UDirectionalLightComponent : public ULightComponentBase
{
	REFLECT_CLASS(UDirectionalLightComponent, ULightComponentBase)

public:
	UDirectionalLightComponent();

	void Serialize(FArchive& Ar) override;
	

	void Deserialize(FArchive& Ar) override;
	

	void SerializeClass(json::JSON& outJson) const override;
	

	void DeserializeClass(const json::JSON& inJson) override;
	

private:
};


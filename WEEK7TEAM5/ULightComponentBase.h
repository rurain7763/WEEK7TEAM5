#pragma once

#include "SceneComponent.h"
#include "Json/json.hpp"
#include "JsonUtil.h"
#include "Vector.h"

class FArchive;
struct FGuid;


class ULightComponentBase : public USceneComponent
{
	REFLECT_CLASS(ULightComponentBase, USceneComponent)

public:
	ULightComponentBase() = default;

	void Serialize(FArchive& Ar) override;


	void Deserialize(FArchive& Ar) override;


	void SerializeClass(json::JSON& outJson) const override;


	void DeserializeClass(const json::JSON& inJson) override;

	// 라이트 아이콘을 만들고 라이트 색으로 칠한다.
	void CreateEditorComponents() override;


	void SetIntensity(float InIntensity);
	float GetIntensity() const;

	void SetColor(const FLinearColor& InColor);
	FLinearColor GetColor() const;

protected:
	// 하위 라이트가 자기 아이콘 텍스처를 고른다. 전용 아이콘이 없으면 PointLightIcon을 쓴다.
	virtual const FGuid& GetEditorIconTextureID() const;

	float Intensity = 3.f;
	FLinearColor LightColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

private:
	void SyncEditorIconColor();
};


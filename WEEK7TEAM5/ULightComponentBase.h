#pragma once

#include "SceneComponent.h"

// Todo: Change header name -> Structs.h
#include "Vector.h"

// Todo: Change to abstract class
class ULightComponentBase : public USceneComponent
{
    REFLECT_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() = default;

    float GetIntensity() const;
    FLinearColor GetLightColor() const;
    bool IsVisible() const;

    void SetIntensity(float InIntensity);
    void SetLightColor(const FLinearColor& InColor);
    void SetVisible(bool bInVisible);

    // Todo: Serialize
    /*
    void Serialize(FArchive& Ar) override;
    void Deserialize(FArchive& Ar) override;

    void SerializeClass(json::JSON& OutJson) const override;
    void DeserializeClass(const json::JSON& InJson) override;
    */
private:
    float mIntensity;
    FLinearColor mLightColor;
    bool mbVisible;
};



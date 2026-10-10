#pragma once

#include "ULightComponentBase.h"
#include "enum.h"

// Todo: Change to abstract class

class ULightComponent : public ULightComponentBase
{
    REFLECT_CLASS(ULightComponent, ULightComponentBase)

public:
    ULightComponent();
    virtual ~ULightComponent() = default;

    ELightType GetLightType() const;
    FVector GetDirection();

protected:
    ULightComponent(ELightType LightType);

private:
    ELightType mLightType;
};

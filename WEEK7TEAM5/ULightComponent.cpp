
#include "ULightComponent.h"

#include "EngineMathLibrary.h"

ULightComponent::ULightComponent()
    : mLightType(ELightType::None)
{
}

ULightComponent::ULightComponent(ELightType LightType)
    : mLightType(LightType)
{
}

ELightType ULightComponent::GetLightType() const
{
    return mLightType;
}

FVector ULightComponent::GetDirection()
{
    FQuaternion Rotation = GetWorldRotation();
    Rotation.Normalize();

    const FMatrix RotationMatrix = ToMatrix(Rotation);

    FVector Direction = RotationMatrix.TransformVector(FVector(1.f, 0.f, 0.f));
    Direction.Normalize();

    return Direction;
}
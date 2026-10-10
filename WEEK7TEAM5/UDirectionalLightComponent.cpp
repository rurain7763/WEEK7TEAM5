
#include "UDirectionalLightComponent.h"

#include "EngineMathLibrary.h"

UDirectionalLightComponent::UDirectionalLightComponent()
    : Super(ELightType::Directional)
{
}

FVector UDirectionalLightComponent::GetDirection()
{
    FQuaternion Rotation = GetWorldRotation();
    Rotation.Normalize();

    const FMatrix RotationMatrix = ToMatrix(Rotation);

    FVector Direction = RotationMatrix.TransformVector(FVector(1.f, 0.f, 0.f));
    Direction.Normalize();

    return Direction;
}

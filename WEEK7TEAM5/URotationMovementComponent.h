#pragma once

#include "ActorComponent.h"
#include "Actor.h"
#include "SceneComponent.h"
#include "Vector.h"
#include "FQuaternion.h"
#include "JsonUtil.h"
#include "FArchive.h"
#include "Serializers.h"

class URotationMovementComponent : public UActorComponent
{
	REFLECT_CLASS(URotationMovementComponent, UActorComponent)

public:
	URotationMovementComponent()
	{
		SetTickable(true);
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		
		Ar << RotationAxis;
		Ar << RotationSpeed;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << RotationAxis;
		Ar << RotationSpeed;
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		outJson["Properties"]["RotationAxis"] = JsonUtils::ToJson(RotationAxis);
		outJson["Properties"]["RotationSpeed"] = RotationSpeed;
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (PropertiesJson.hasKey("RotationAxis"))
		{
			RotationAxis = JsonUtils::FromJson<FVector>(PropertiesJson.at("RotationAxis"));
		}
		if (PropertiesJson.hasKey("RotationSpeed"))
		{
			RotationSpeed = JsonUtils::FromJson<float>(PropertiesJson.at("RotationSpeed"));
		}
	}

	void Tick(float DeltaTime) override
	{
		if (!mOwner)
		{
			return;
		}

		USceneComponent* RootComp = mOwner->GetRootComponent();
		if (!RootComp)
		{
			return;
		}

		FQuaternion CurrentRotation = RootComp->GetWorldRotation();
		FQuaternion DeltaRotation = FQuaternion(RotationAxis, RotationSpeed * DeltaTime);
		FQuaternion NewRotation = DeltaRotation * CurrentRotation;
		NewRotation.Normalize();
		RootComp->SetWorldRotation(NewRotation);
	}

	inline void SetRotationAxis(const FVector& InRotationAxis)
	{
		RotationAxis = InRotationAxis;
		RotationAxis.Normalize();
	}

	inline FVector GetRotationAxis() const { return RotationAxis; }

	inline void SetRotationSpeed(float InRotationSpeed) { RotationSpeed = InRotationSpeed; }
	inline float GetRotationSpeed() const { return RotationSpeed; }

private:
	FVector RotationAxis = FVector(0.f, 0.f, 1.f);
	float RotationSpeed = 1.f;
};

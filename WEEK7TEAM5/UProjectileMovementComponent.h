#pragma once

#include "ActorComponent.h"
#include "Actor.h"
#include "SceneComponent.h"
#include "Vector.h"
#include "JsonUtil.h"
#include "FArchive.h"
#include "Serializers.h"

class UProjectileMovementComponent : public UActorComponent
{
	REFLECT_CLASS(UProjectileMovementComponent, UActorComponent)

public:
	UProjectileMovementComponent()
	{
		SetTickable(true);
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Velocity;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);
		Ar << Velocity;
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		outJson["Properties"]["Velocity"] = JsonUtils::ToJson(Velocity);
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (PropertiesJson.hasKey("Velocity"))
		{
			Velocity = JsonUtils::FromJson<FVector>(PropertiesJson.at("Velocity"));
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

		FVector CurrentLocation = RootComp->GetWorldLocation();
		FVector NewLocation = CurrentLocation + Velocity * DeltaTime;
		RootComp->SetWorldLocation(NewLocation);
	}

	inline void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }
	inline FVector GetVelocity() const { return Velocity; }

private:
	FVector Velocity = FVector(1.f, 0.f, 0.f);
};

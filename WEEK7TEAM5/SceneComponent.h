#pragma once

#include "ActorComponent.h"
#include "GraphicsManager.h"
#include "FQuaternion.h"
#include "Vector.h"

struct FTransform;
class UBillboardComponent;

class USceneComponent : public UActorComponent
{
	REFLECT_CLASS(USceneComponent, UActorComponent)

public:
	USceneComponent() = default;
	virtual ~USceneComponent() = default;

	void Initialize(FVector location, FRotator rotation, FVector scale3D);

	void BeginDestroy() override;

	virtual void Serialize(FArchive& Ar) override;
	virtual void Deserialize(FArchive& Ar) override;
	virtual void SerializeClass(json::JSON& outJson) const override;
	virtual void DeserializeClass(const json::JSON& inJson) override;

	void SetupAttachment(USceneComponent* ParentComponent, bool KeepWorldTransform = true);
	void DetachFromParent(bool KeepWorldTransform = true);

	FVector GetRelativeLocation() const;
	void SetRelativeLocation(FVector location);
	void SetWorldLocation(FVector WorldLocation);
	FVector GetWorldLocation();

	FQuaternion GetRelativeRotation() const;
	void SetRelativeRotation(FQuaternion rotation);
	void SetWorldRotation(FQuaternion WorldRotation);
	FQuaternion GetWorldRotation();

	FVector GetRelativeScale3D() const;
	void SetRelativeScale3D(FVector scale);

	const FTransform& GetTransform() const;

	const FMatrix& GetWorldMatrix() const;

	const inline FVector GetForwardVector()
	{
		FMatrix Matrix = GetWorldMatrix();
		FVector Axis = Matrix.GetUnitAxis(EAxis::X);
		Axis.Normalize();
		return Axis;
	}

	inline bool HasParent() const { return mParentComponent != nullptr; }
	inline USceneComponent* GetParentComponent() const { return mParentComponent; }
	inline const TArray<USceneComponent*>& GetChildComponents() const { return mChildComponents; }

	FVector GetForwardVector() const;	

protected:
	virtual void OnTransformChanged() {}

	// 이 컴포넌트에 붙는 에디터 전용 아이콘 빌보드를 만듭니다. 이미 있으면 기존 아이콘을 돌려줍니다.
	// 아이콘은 이 컴포넌트에 Attach되고, 이 컴포넌트가 소멸되면 함께 제거됩니다.
	UBillboardComponent* CreateEditorIcon(const FGuid& IconTextureID, const FName& IconName);
	UBillboardComponent* GetEditorIcon() const;

private:
	bool CanAttachTo(USceneComponent* ParentComponent) const;

	void PostWorldMatrixChanged();

private:
	FTransform mRelativeTransform;

	mutable bool mbWorldMatrixDirty = true;
	mutable FMatrix mWorldMatrix = FMatrix::Identity;

	USceneComponent* mParentComponent = nullptr;
	TArray<USceneComponent*> mChildComponents;

	// CreateEditorIcon으로 만든 아이콘. 아이콘이 먼저 소멸되면 아이콘 쪽에서 비운다.
	USceneComponent* mEditorIcon = nullptr;

	FVector CacheForwardVector;
};


#pragma once

#include "PrimitiveComponent.h"
#include "Assets.h"
#include "FAssetManager.h"
#include "RenderInfo.h"
#include "Camera.h"
#include "FQuaternion.h"
#include "JsonUtil.h"
#include "FArchive.h"
#include "Serializers.h"

class UBillboardComponent : public UPrimitiveComponent
{
	REFLECT_CLASS(UBillboardComponent, UPrimitiveComponent)

public:
	UBillboardComponent()
	{
		SetTickable(true);
		bTickInEditor = true;

		mMeshAsset = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(FName("PlaneMesh"), true);
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		FGuid MeshAssetID = mMeshAsset ? mMeshAsset->GetAssetID() : FGuid();
		Ar << MeshAssetID;

		FGuid TextureAssetID = mTextureAsset ? mTextureAsset->GetAssetID() : FGuid();
		Ar << TextureAssetID;

		Ar << mSubUV;
		Ar << mSubUVOffset;
		Ar << mBlendMode;
		Ar << mEnableDepthTest;
		Ar << mEnableDepthWrite;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		FGuid MeshAssetID;
		Ar << MeshAssetID;
		mMeshAsset = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(MeshAssetID, true);

		FGuid TextureAssetID;
		Ar << TextureAssetID;
		mTextureAsset = FAssetManager::Get().GetAssetAs<FTexture2DAsset>(TextureAssetID, true);

		Ar << mSubUV;
		Ar << mSubUVOffset;
		Ar << mBlendMode;
		Ar << mEnableDepthTest;
		Ar << mEnableDepthWrite;
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		UPrimitiveComponent::SerializeClass(outJson);

		if (mTextureAsset)
		{
			FGuid AssetID = mTextureAsset->GetAssetID();
			outJson["Properties"]["ObjTextureAsset"] = JsonUtils::ToJson(AssetID);
		}
		outJson["Properties"]["mSubUVOffset"] = JsonUtils::ToJson(mSubUVOffset);
		outJson["Properties"]["mEnableDepthTest"] = mEnableDepthTest;
		outJson["Properties"]["mEnableDepthWrite"] = mEnableDepthWrite;
		outJson["Properties"]["mBlendMode"] = static_cast<int>(mBlendMode);
		outJson["Properties"]["mSubUV"] = JsonUtils::ToJson(mSubUV);
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		UPrimitiveComponent::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (PropertiesJson.hasKey("ObjTextureAsset"))
		{
			if (PropertiesJson.at("ObjTextureAsset").JSONType() == json::JSON::Class::Object)
			{
				FGuid AssetID = JsonUtils::FromJson<FGuid>(PropertiesJson.at("ObjTextureAsset"));
				if (AssetID.IsValid())
				{
					mTextureAsset = FAssetManager::Get().GetAssetAs<FTexture2DAsset>(AssetID, true);
				}
			}
		}

		if (PropertiesJson.hasKey("mSubUVOffset"))
		{
			mSubUVOffset = JsonUtils::FromJson<FVector2>(PropertiesJson.at("mSubUVOffset"));
		}
		if (PropertiesJson.hasKey("mEnableDepthTest"))
		{
			mEnableDepthTest = JsonUtils::FromJson<bool>(PropertiesJson.at("mEnableDepthTest"));
		}
		if (PropertiesJson.hasKey("mEnableDepthWrite"))
		{
			mEnableDepthWrite = JsonUtils::FromJson<bool>(PropertiesJson.at("mEnableDepthWrite"));
		}
		if (PropertiesJson.hasKey("mBlendMode"))
		{
			mBlendMode = static_cast<ERenderBlendMode>(JsonUtils::FromJson<int>(PropertiesJson.at("mBlendMode")));
		}
		if (PropertiesJson.hasKey("mSubUV"))
		{
			mSubUV = JsonUtils::FromJson<FVector4>(PropertiesJson.at("mSubUV"));
		}

	}

	void Tick(float DeltaTime) override
	{
		MarkRenderDirty();
	}

	void Render(FRenderCollector& RenderCollector) override
	{
		Super::Render(RenderCollector);

		mRenderProxy->SetCollector(RenderCollector);
		mRenderProxy->ReserveRenderTransparentQuadInfos(1);

		FMatrix PivotMatrix = GetWorldMatrix();

		FMatrix TranslationMatrix = FMatrix::ExtractTranslation(PivotMatrix);
		FVector Translation = FMatrix::GetTranslation(PivotMatrix);
		FMatrix ScaleMatrix = FMatrix::ExtractScaleMatrix(PivotMatrix);
		FQuaternion BillboardRotation = RenderCollector.Camera->Transform.GetRotation();

		PivotMatrix = ScaleMatrix * ToMatrix(BillboardRotation) * FMatrix::Translation(Translation);

		FRenderQuadInfo& QuadInfo = mRenderProxy->GetRenderTransparentQuadInfo(0);
		QuadInfo.Model = PivotMatrix;
		QuadInfo.Color = FVector4(1.f, 1.f, 1.f, 1.f);
		QuadInfo.TextureSRV = mTextureAsset ? mTextureAsset->GetSRV() : nullptr;
		QuadInfo.TextureFormat = mTextureAsset ? mTextureAsset->GetFormat() : DXGI_FORMAT_UNKNOWN;
		QuadInfo.SubUV = mSubUV + FVector4(mSubUVOffset.X, mSubUVOffset.Y, 0.f, 0.f);
		QuadInfo.BlendMode = mBlendMode;
		QuadInfo.EnableDepthTest = mEnableDepthTest;
		QuadInfo.EnableDepthWrite = mEnableDepthWrite;

		mRenderProxy->SetActiveRenderTransparentQuadInfoNum(1);
	}

	FAABB GetBoundingBox() override
	{
		if (!mMeshAsset)
		{
			return FAABB();
		}

		const FMatrix& WorldMatrix = GetWorldMatrix();
		return mMeshAsset->GetLocalBoundingBox().ToWorld(WorldMatrix);
	}

	const TArray<FVertex>& GetMeshVertices() const override
	{
		if (!mMeshAsset)
		{
			return UPrimitiveComponent::GetMeshVertices();
		}
		return mMeshAsset->GetVertices();
	}

	const TArray<uint32>& GetMeshIndices() const override
	{
		if (!mMeshAsset)
		{
			return UPrimitiveComponent::GetMeshIndices();
		}

		return mMeshAsset->GetIndices();
	}

	inline void SetTexture(const TSharedPtr<FTexture2DAsset>& textureAsset) { mTextureAsset = textureAsset; }
	inline const TSharedPtr<FTexture2DAsset>& GetTexture() const { return mTextureAsset; }

	inline void SetDepthState(bool enableDepthTest, bool enableDepthWrite) { mEnableDepthTest = enableDepthTest; mEnableDepthWrite = enableDepthWrite; }
	void SetBlendState(ERenderBlendMode InBlendMode) { mBlendMode = InBlendMode; }

	inline FVector4 GetSubUV() { return mSubUV; }
	void SetSubUV(FVector4 InmSubUV) { mSubUV = InmSubUV; }

	inline FVector2 GetSubUVOffset() { return mSubUVOffset; }
	void SetSubUVOffset(FVector2 InmSubUVOffset) { mSubUVOffset = InmSubUVOffset; }

	inline void SetEnbaleDepthTest(bool InEnableDepthTest) { mEnableDepthTest = InEnableDepthTest; }
	inline bool GetEnbaleDepthTest() { return mEnableDepthTest; }

	inline void SetEnbaleDepthWrite(bool InEnableDepthWrite) { mEnableDepthWrite = InEnableDepthWrite; }
	inline bool GetEnbaleDepthWrite() { return mEnableDepthWrite; }

	inline void SetRenderBlendMode(ERenderBlendMode InBlendMode) { mBlendMode = InBlendMode; }
	inline ERenderBlendMode GetRenderBlendMode() { return mBlendMode; }

protected:
	TSharedPtr<FStaticMeshAsset> mMeshAsset;
	TSharedPtr<FTexture2DAsset> mTextureAsset;
	FVector4 mSubUV = { 0.f, 0.f, 1.f, 1.f };
	FVector2 mSubUVOffset = { 0.f, 0.f };

	ERenderBlendMode mBlendMode = ERenderBlendMode::Transparent;
	bool mEnableDepthTest = true;
	bool mEnableDepthWrite = true;
};

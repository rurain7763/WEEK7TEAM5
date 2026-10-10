#pragma once

#include "PrimitiveComponent.h"
#include "Assets.h"
#include "FAssetManager.h"
#include "RenderInfo.h"
#include "JsonUtil.h"
#include "FArchive.h"
#include "Serializers.h"

class UPlaneComponent : public UPrimitiveComponent
{
	REFLECT_CLASS(UPlaneComponent, UPrimitiveComponent)

public:
	UPlaneComponent()
	{
        SetTickable(true);
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
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		UPrimitiveComponent::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (!PropertiesJson.hasKey("ObjTextureAsset"))
		{
			throw std::runtime_error("UPlaneComponent: ObjTextureAsset property is required");
		}

		if (PropertiesJson.at("ObjTextureAsset").JSONType() != json::JSON::Class::Object)
		{
			throw std::runtime_error("UPlaneComponent: ObjTextureAsset property requires an object");
		}

		FGuid AssetID = JsonUtils::FromJson<FGuid>(PropertiesJson.at("ObjTextureAsset"));
		if (AssetID.IsValid())
		{
			mTextureAsset = FAssetManager::Get().GetAssetAs<FTexture2DAsset>(AssetID, true);
		}
	}

	void Render(FRenderCollector& RenderCollector) override
	{
		Super::Render(RenderCollector);

		mRenderProxy->SetCollector(RenderCollector);
		mRenderProxy->ReserveRenderTransparentQuadInfos(1);

		FMatrix PivotMatrix = GetWorldMatrix();

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

	inline void SetTexture(const TSharedPtr<FTexture2DAsset>& textureAsset) 
	{ 
		mTextureAsset = textureAsset; 
		MarkRenderDirty();
	}

	inline const TSharedPtr<FTexture2DAsset>& GetTexture() const { return mTextureAsset; }

	inline void SetDepthState(bool enableDepthTest, bool enableDepthWrite)
	{ 
		mEnableDepthTest = enableDepthTest; 
		mEnableDepthWrite = enableDepthWrite; 
		MarkRenderDirty();
	}

	inline void SetBlendState(ERenderBlendMode InBlendMode) 
	{ 
		mBlendMode = InBlendMode; 
		MarkRenderDirty();
	}

protected:
	TSharedPtr<FStaticMeshAsset> mMeshAsset;
	TSharedPtr<FTexture2DAsset> mTextureAsset;
	FVector4 mSubUV = { 0.f, 0.f, 1.f, 1.f };
	FVector2 mSubUVOffset = { 0.f, 0.f };

	ERenderBlendMode mBlendMode = ERenderBlendMode::Opaque;
	bool mEnableDepthTest = true;
	bool mEnableDepthWrite = true;
};

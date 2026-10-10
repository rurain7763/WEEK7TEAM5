#pragma once

#include "SceneComponent.h"
#include "PrimitiveComponent.h"
#include "Assets.h"
#include "Camera.h"
#include "Actor.h"
#include "FAssetManager.h"
#include "ShowFlags.h"
#include "MathUtility.h"
#include "Json/json.hpp"
#include "JsonUtil.h"
#include "FTextBuilder.h"
#include "FQuaternion.h"
#include "FArchive.h"
#include "Serializers.h"
#include "Vector.h"

class USpotLightComponent : public USceneComponent
{
	REFLECT_CLASS(USpotLightComponent, USceneComponent)

public:
    USpotLightComponent() { SetTickable(true); }

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		Ar << Range;
		Ar << mInnerConeAngle;
		Ar << mOuterConeAngle;
		Ar << mColor;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << Range;
		Ar << mInnerConeAngle;
		Ar << mOuterConeAngle;
		Ar << mColor;
	}

	inline float GetRange() const { return Range; }
	inline float GetInnerConeAngle() const { return mInnerConeAngle; }
	inline float GetOuterConeAngle() const { return mOuterConeAngle; }
	inline const FVector4& GetColor() const { return mColor; }

	inline void SetColor(const FVector4& InColor) { mColor = InColor; }

	inline void SetOuterConeAngle(float InAngle)
	{
		mOuterConeAngle = FMath::Clamp(InAngle, 0.f, MAX_CONE_ANGLE);
		mInnerConeAngle = FMath::Min(mInnerConeAngle, mOuterConeAngle);
	}

	inline void SetInnerConeAngle(float InAngle)
	{
		mInnerConeAngle = FMath::Clamp(InAngle, 0.f, mOuterConeAngle);
	}

private:
	static constexpr float MAX_CONE_ANGLE = 89.f;

	float Range = 5.0f;
	FVector4 mColor = { 1.f, 1.f, 1.f, 1.f };
	float mInnerConeAngle = 30.0f;
	float mOuterConeAngle = 45.0f;
};

class UText3DComponent : public USceneComponent
{
	REFLECT_CLASS(UText3DComponent, USceneComponent)

public:
	UText3DComponent()
	{
		SetRenderable(true);
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		Ar << mbBillboard;
		Ar << mText;
		
		FGuid FontAtlasAssetID = mFontAtlasAsset ? mFontAtlasAsset->GetAssetID() : FGuid();
		Ar << FontAtlasAssetID;

		Ar << mColor;
		Ar << mEnableDepthTest;
		Ar << mEnableDepthWrite;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);
		
		Ar << mbBillboard;
		Ar << mText;

		FGuid FontAtlasAssetID;
		Ar << FontAtlasAssetID;

		mFontAtlasAsset = FAssetManager::Get().GetAssetAs<FFontAtlasAsset>(FontAtlasAssetID, true);

		Ar << mColor;
		Ar << mEnableDepthTest;
		Ar << mEnableDepthWrite;
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		// std::wstring을 UTF-8 문자열로 변환하여 저장
		outJson["Properties"]["mText"] = Wide2Utf(mText).CStr();
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& propertiesJson = inJson.at("Properties");

		// 이전 버전 씬 파일과의 호환성을 위해 필수가 아닌 값으로 처리
		if (propertiesJson.hasKey("mText") && propertiesJson.at("mText").JSONType() == json::JSON::Class::String)
		{
			mText = Utf2Wide(FString(propertiesJson.at("mText").ToString()));
		}
		else
		{
			mText.clear();
		}
	}

	void Render(FRenderCollector& RenderCollector) override
	{
		Super::Render(RenderCollector);

		mRenderProxy->SetCollector(RenderCollector);
		mRenderProxy->SetActiveRenderOverlayQuadInfoNum(0);

		// Show Flags에서 끄면 쿼드를 아예 만들지 않는다.
		// 만들고 거르는 게 아니라 글자 수만큼의 계산 자체가 사라진다.
		if (!FShowFlags::Get().IsEnabled(EShowFlag::UUIDText))
		{
			return;
		}

        if (!mOwner || !mOwner->GetRootComponent()) return;

		if (!mFontAtlasAsset)
		{
			return;
		}

		const TSharedPtr<FFontAtlas>& FontAtlas = mFontAtlasAsset->GetFontAtlas();
		if (!FontAtlas || mText.empty())
		{
			return;
		}

		mRenderProxy->ReserveRenderOverlayQuadInfos(static_cast<int32>(mText.length()));

		FTextBuilder TextBuilder(FontAtlas, WorldUnitPerPixel);

		float TotalWidth = 0.0f;
		float TotalHeight = 0.0f;
		TextBuilder.CalculateSize(mText, TotalWidth, TotalHeight);

		FMatrix PivotMatrix = GetWorldMatrix();

		if (mbBillboard && RenderCollector.Camera)
		{
			FMatrix TranslationMatrix = FMatrix::ExtractTranslation(PivotMatrix);
			FVector Translation = FMatrix::GetTranslation(PivotMatrix);
			FQuaternion BillboardRotation = RenderCollector.Camera->Transform.GetRotation();

			UPrimitiveComponent* Primitive = HasParent() ? GetParentComponent()->Cast<UPrimitiveComponent>() : nullptr;
			if (Primitive)
			{
				const FAABB Bounds = Primitive->GetBoundingBox();
				Translation = FVector(Translation.x, Translation.y, Bounds.Max.z + 0.2f);
			}

			PivotMatrix = ToMatrix(BillboardRotation) * FMatrix::Translation(Translation);
		}

		int32 GlyphCount = 0;
		TextBuilder.Build(mText, TotalWidth, TotalHeight, [&](const FRect& Rect, const FRect& UV) {
			// 공백 등은 Builder에서 advance만 적용하고, 쿼드는 생략한다.
			if (Rect.Width <= 0.f || Rect.Height <= 0.f)
			{
				return;
			}

			const FVector GlyphCenter(0.f, Rect.X, Rect.Y);

			FRenderQuadInfo& QuadInfo = mRenderProxy->GetRenderOverlayQuadInfo(GlyphCount);
			QuadInfo.Model = FMatrix::Scale(FVector3(1.f, Rect.Width, Rect.Height)) * FMatrix::Translation(GlyphCenter) * PivotMatrix;
			QuadInfo.Color = mColor;
			QuadInfo.TextureSRV = mFontAtlasAsset->GetSRV();
			QuadInfo.TextureFormat = mFontAtlasAsset->GetFormat();
			QuadInfo.SubUV = FVector4(UV.X, UV.Y, UV.Width, UV.Height);
			QuadInfo.BlendMode = ERenderBlendMode::Transparent;
			QuadInfo.EnableDepthTest = mEnableDepthTest;
			QuadInfo.EnableDepthWrite = mEnableDepthWrite;

			GlyphCount++;
		});

		mRenderProxy->SetActiveRenderOverlayQuadInfoNum(GlyphCount);
	}

	inline void SetBillboard(bool billboard) { mbBillboard = billboard; }

	inline void SetText(const std::wstring& text) { mText = text; }
	inline const std::wstring& GetText() const { return mText; }

	inline void SetFontAtlasAsset(const TSharedPtr<FFontAtlasAsset>& fontAtlasAsset) { mFontAtlasAsset = fontAtlasAsset; }

	inline void SetColor(const FVector4& color) { mColor = color; }
	inline void SetDepthState(bool enableDepthTest, bool enableDepthWrite) { mEnableDepthTest = enableDepthTest; mEnableDepthWrite = enableDepthWrite; }

private:
	bool mbBillboard = false;
	std::wstring mText;
	TSharedPtr<FFontAtlasAsset> mFontAtlasAsset;
	FVector4 mColor = FVector4(1, 1, 1, 1);
	bool mEnableDepthTest = true;
	bool mEnableDepthWrite = true;
};


class UPointLightComponent : public USceneComponent
{
	REFLECT_CLASS(UPointLightComponent, USceneComponent)

public:
	UPointLightComponent() = default;

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		
		Ar << Intensity;
		Ar << Radius;
		Ar << RadiusFallOff;
		Ar << Color;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << Intensity;
		Ar << Radius;
		Ar << RadiusFallOff;
		Ar << Color;
	}

	void SerializeClass(json::JSON& outJson) const override
	{
		Super::SerializeClass(outJson);

		outJson["Properties"]["Intensity"] = Intensity;
		outJson["Properties"]["Radius"] = Radius;
		outJson["Properties"]["RadiusFallOff"] = RadiusFallOff;
		outJson["Properties"]["Color"] = JsonUtils::ToJson(Color);
	}

	void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& PropertiesJson = inJson.at("Properties");
		if (PropertiesJson.hasKey("Intensity"))
		{
			Intensity = JsonUtils::FromJson<float>(PropertiesJson.at("Intensity"));
		}
		if (PropertiesJson.hasKey("Radius"))
		{
			Radius = JsonUtils::FromJson<float>(PropertiesJson.at("Radius"));
		}
		if (PropertiesJson.hasKey("RadiusFallOff"))
		{
			RadiusFallOff = JsonUtils::FromJson<float>(PropertiesJson.at("RadiusFallOff"));
		}
		if (PropertiesJson.hasKey("Color"))
		{
			Color = JsonUtils::FromJson<FLinearColor>(PropertiesJson.at("Color"));
		}
	}

	inline void SetIntensity(float InIntensity) { Intensity = InIntensity; }
	inline float GetIntensity() const { return Intensity; }

	inline void SetRadius(float InRadius) { Radius = InRadius; }
	inline float GetRadius() const { return Radius; }

	inline void SetRadiusFallOff(float InRadiusFallOff) { RadiusFallOff = InRadiusFallOff; }
	inline float GetRadiusFallOff() const { return RadiusFallOff; }

	inline void SetColor(const FLinearColor& InColor) { Color = InColor; }
	inline FLinearColor GetColor() const { return Color; }

private:
	float Intensity = 3.f;
	float Radius = 5.f;
	float RadiusFallOff = 1.f;
	FLinearColor Color = FLinearColor(1.f, 1.f, 1.f, 1.f);
};


class UTextRenderComponent : public UPrimitiveComponent
{
	REFLECT_CLASS(UTextRenderComponent, UPrimitiveComponent)

public:
	UTextRenderComponent()
	{
		SetRenderable(true);
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		Ar << mText;

		FGuid FontAtlasAssetID = mFontAtlasAsset ? mFontAtlasAsset->GetAssetID() : FGuid();
		Ar << FontAtlasAssetID;

		Ar << mColor;
		Ar << mEnableDepthTest;
		Ar << mEnableDepthWrite;
	}

	virtual void Deserialize(FArchive& Ar) override
	{
		Super::Deserialize(Ar);

		Ar << mText;

		FGuid FontAtlasAssetID;
		Ar << FontAtlasAssetID;

		mFontAtlasAsset = FAssetManager::Get().GetAssetAs<FFontAtlasAsset>(FontAtlasAssetID, true);

		Ar << mColor;
		Ar << mEnableDepthTest;
		Ar << mEnableDepthWrite;
	}

	virtual void SerializeClass(json::JSON& OutJson) const override
	{
		Super::SerializeClass(OutJson);

		OutJson["Properties"]["mText"] = JsonUtils::ToJson(mText);
		OutJson["Properties"]["mColor"] = JsonUtils::ToJson(mColor);
	}

	virtual void DeserializeClass(const json::JSON& inJson) override
	{
		Super::DeserializeClass(inJson);

		const json::JSON& propertiesJson = inJson.at("Properties");
		
		mText = JsonUtils::FromJson<std::wstring>(propertiesJson.at("mText"));
		mColor = JsonUtils::FromJson<FVector4>(propertiesJson.at("mColor"));
		mFontAtlasAsset = FAssetManager::Get().GetAssetAs<FFontAtlasAsset>(FName("TestFontAtlas"), true);
	}

	void Render(FRenderCollector& RenderCollector) override
	{
		Super::Render(RenderCollector);

		mRenderProxy->SetCollector(RenderCollector);
		mRenderProxy->SetActiveRenderTransparentQuadInfoNum(0);

		if (!mOwner || !mOwner->GetRootComponent())
		{
			return;
		}

		if (!mFontAtlasAsset)
		{
			return;
		}

		const TSharedPtr<FFontAtlas>& FontAtlas = mFontAtlasAsset->GetFontAtlas();
		if (!FontAtlas || mText.empty())
		{
			return;
		}

		mRenderProxy->ReserveRenderTransparentQuadInfos(static_cast<int32>(mText.length()));

		FTextBuilder TextBuilder(FontAtlas, WorldUnitPerPixel);

		float TotalWidth = 0.0f;
		float TotalHeight = 0.0f;
		TextBuilder.CalculateSize(mText, TotalWidth, TotalHeight);

		FMatrix PivotMatrix = GetWorldMatrix();

		int32 GlyphCount = 0;
		TextBuilder.Build(mText, TotalWidth, TotalHeight, [&](const FRect& Rect, const FRect& UV) {
			// 공백 등은 Builder에서 advance만 적용하고, 쿼드는 생략한다.
			if (Rect.Width <= 0.f || Rect.Height <= 0.f)
			{
				return;
			}

			const FVector GlyphCenter(0.f, Rect.X, Rect.Y);

			FRenderQuadInfo& QuadInfo = mRenderProxy->GetRenderTransparentQuadInfo(GlyphCount);
			QuadInfo.Model = FMatrix::Scale(FVector3(1.f, Rect.Width, Rect.Height)) * FMatrix::Translation(GlyphCenter) * PivotMatrix;
			QuadInfo.Color = mColor;
			QuadInfo.TextureSRV = mFontAtlasAsset->GetSRV();
			QuadInfo.TextureFormat = mFontAtlasAsset->GetFormat();
			QuadInfo.SubUV = FVector4(UV.X, UV.Y, UV.Width, UV.Height);
			QuadInfo.BlendMode = ERenderBlendMode::Transparent;
			QuadInfo.EnableDepthTest = mEnableDepthTest;
			QuadInfo.EnableDepthWrite = mEnableDepthWrite;

			GlyphCount++;
		});

		mRenderProxy->SetActiveRenderTransparentQuadInfoNum(GlyphCount);
	}

	inline void SetText(const std::wstring& text)
	{
		mText = text;
		mbBoundingBoxDirty = true;
		MarkBoundsDirty();
		MarkRenderDirty();
	}

	inline const std::wstring& GetText() const { return mText; }

	inline void SetFontAtlasAsset(const TSharedPtr<FFontAtlasAsset>& fontAtlasAsset) 
	{ 
		mFontAtlasAsset = fontAtlasAsset; 
		mbBoundingBoxDirty = true;
		MarkBoundsDirty();
		MarkRenderDirty();
	}

	inline void SetColor(const FVector4& color)
	{
		mColor = color;
		MarkRenderDirty();
	}

	inline void SetDepthState(bool enableDepthTest, bool enableDepthWrite)
	{
		mEnableDepthTest = enableDepthTest;
		mEnableDepthWrite = enableDepthWrite;
		MarkRenderDirty();
	}

protected:
	virtual FAABB GetBoundingBox() override
	{
		if (mbBoundingBoxDirty)
		{
			mbBoundingBoxDirty = false;

			if (!mFontAtlasAsset)
			{
				mBoundingBox = FAABB();
				return mBoundingBox;
			}

			const TSharedPtr<FFontAtlas>& FontAtlas = mFontAtlasAsset->GetFontAtlas();
			if (!FontAtlas)
			{
				mBoundingBox = FAABB();
				return mBoundingBox;
			}

			FTextBuilder TextBuilder(FontAtlas, WorldUnitPerPixel);

			float TotalWidth = 0.0f;
			float TotalHeight = 0.0f;
			TextBuilder.CalculateSize(mText, TotalWidth, TotalHeight);

			mBoundingBox.Min = FVector(-0.5f, -TotalWidth * 0.5f, -TotalHeight * 0.5f);
			mBoundingBox.Max = FVector(0.5f, TotalWidth * 0.5f, TotalHeight * 0.5f);
			mBoundingBox = mBoundingBox.ToWorld(GetWorldMatrix());
		}

		return mBoundingBox;
	}

	virtual const TArray<FVertex>& GetMeshVertices() const override
	{
		static TArray<FVertex> Vertices = {
			FVertex{ FVector(0.f, -0.5f, 0.5f), FVector(0.f, 0.f, 1.f), mColor, FVector2(0.f, 0.f) },
			FVertex{ FVector(0.f, 0.5f, 0.5f), FVector(0.f, 0.f, 1.f), mColor, FVector2(1.f, 0.f) },
			FVertex{ FVector(0.f, 0.5f, -0.5f), FVector(0.f, 0.f, 1.f), mColor, FVector2(1.f, 1.f) },
			FVertex{ FVector(0.f, -0.5f, -0.5f), FVector(0.f, 0.f, 1.f), mColor, FVector2(0.f, 1.f) },
		};

		return Vertices;
	}

	virtual const TArray<uint32>& GetMeshIndices() const override
	{
		static TArray<uint32> Indices = {
			0, 1, 2,
			0, 2, 3
		};

		return Indices;
	}

	virtual void OnTransformChanged() override
	{
		mbBoundingBoxDirty = true;
		Super::OnTransformChanged();
	}

private:
	std::wstring mText;
	TSharedPtr<FFontAtlasAsset> mFontAtlasAsset;
	FVector4 mColor = FVector4(1, 1, 1, 1);
	bool mEnableDepthTest = true;
	bool mEnableDepthWrite = true;

	bool mbBoundingBoxDirty = true;
	FAABB mBoundingBox;
};


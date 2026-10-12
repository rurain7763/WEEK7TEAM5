#pragma once

#include "Core.h"
#include "FAssetEditor.h"
#include "FAssetManager.h"
#include "Assets.h"
#include "AssetFileIOs.h"
#include "FArchive.h"
#include "ImGui/imgui.h"
#include "FEditorUtils.hpp"
#include <format>

class FMaterialEditor : public FAssetEditor
{
public:
	FMaterialEditor(const FAssetMetaInfo& InAssetMetaInfo, const std::filesystem::path& InAssetFilePath)
		: AssetFilePath(InAssetFilePath)
	{
		MaterialAsset = FAssetManager::Get().GetAssetAs<FMaterialAsset>(InAssetMetaInfo.AssetID, true);
		if (!MaterialAsset)
		{
			return;
		}

		NewPayload.AmbientColor = MaterialAsset->GetAmbientColor();
		NewPayload.DiffuseColor = MaterialAsset->GetDiffuseColor();
		NewPayload.SpecularColor = MaterialAsset->GetSpecularColor();
		NewPayload.DiffuseTexture = MaterialAsset->HasDiffuseTexture() ? MaterialAsset->GetDiffuseTexture()->GetAssetID() : FGuid();
		NewPayload.SpecularTexture = MaterialAsset->HasSpecularTexture() ? MaterialAsset->GetSpecularTexture()->GetAssetID() : FGuid();
		NewPayload.NormalTexture = MaterialAsset->HasNormalTexture() ? MaterialAsset->GetNormalTexture()->GetAssetID() : FGuid();
		NewPayload.Opacity = MaterialAsset->GetOpacity();

		bIsOpen = true;
	}

	virtual void Render() override
	{
		FString EditorTitle = std::format("Material Editor - {}##{}", MaterialAsset->GetAssetName().ToString(), MaterialAsset->GetAssetID().ToString());

		ImGui::Begin(EditorTitle.c_str(), &bIsOpen, ImGuiWindowFlags_AlwaysAutoResize);

		bIsDirty |= FEditorUtils::DrawAssetCombo("Diffuse Texture", EAssetType::Texture2D, NewPayload.DiffuseTexture);
		bIsDirty |= FEditorUtils::DrawAssetCombo("Specular Texture", EAssetType::Texture2D, NewPayload.SpecularTexture);
		bIsDirty |= FEditorUtils::DrawAssetCombo("Normal Texture", EAssetType::Texture2D, NewPayload.NormalTexture);

		if (bIsDirty && ImGui::Button("Save"))
		{
			Save();
		}

		ImGui::End();
	}

	virtual void Save() override
	{
		if (!bIsDirty)
		{
			return;
		}

		MaterialAsset->SetDiffuseTexture(NewPayload.DiffuseTexture);
		MaterialAsset->SetSpecularTexture(NewPayload.SpecularTexture);
		MaterialAsset->SetNormalTexture(NewPayload.NormalTexture);
		
		FWindowsBinWriter Writer(AssetFilePath);

		FAssetFileHeader Header;
		Header.Version = 1;
		Header.AssetID = MaterialAsset->GetAssetID();
		Header.AssetType = EAssetType::Material;

		Writer << Header;
		FMaterialFileIO::Save(Writer, NewPayload);

		bIsDirty = false;
	}

private:
	std::filesystem::path AssetFilePath;
	TSharedPtr<FMaterialAsset> MaterialAsset;

	bool bIsDirty = false;
	FMaterialPayload NewPayload;
};
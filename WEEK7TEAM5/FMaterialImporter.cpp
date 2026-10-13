#include "FMaterialImporter.h"
#include "AssetFileIOs.h"
#include "FArchive.h"
#include "FLogManager.h"
#include "FGuid.h"
#include "Serializers.h"
#include "FTexture2DImporter.h"
#include "FileManager.h"

bool FMaterialImporter::ImportDefault(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
    try
    {
	    OutHead.Version = 1;
	    OutHead.AssetType = EAssetType::Material;
	    OutHead.AssetID = FGuid::NewGuid();

        FWindowsBinWriter FileWriter(OutPath);

        FMaterialPayload Payload;
        Payload.AmbientColor = FVector3(0.1, 0.1, 0.1);
	    Payload.DiffuseColor = FVector3(1.0, 1.0, 1.0);
	    Payload.SpecularColor = FVector3(0.0, 0.0, 0.0);
        Payload.DiffuseTexture = FGuid();
        Payload.SpecularTexture = FGuid();
        Payload.NormalTexture = FGuid();
        Payload.Opacity = 1.0f;

        FileWriter << OutHead;

        FMaterialFileIO::Save(FileWriter, Payload);
    }
	catch (const std::exception&)
	{
		UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
		return false;
	}

    return true;
}

bool FMaterialImporter::Import(const FMeshMaterial& Material, const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
	TArray<FMeshMaterial> Materials;
	Materials.Add(Material);

	FMaterialImportResult MaterialsResult;
	if (!Import(Materials, InPath, OutPath.parent_path(), MaterialsResult))
	{
		return false;
	}

	OutHead = MaterialsResult.Headers[0];

	return true;
}

bool FMaterialImporter::Import(const TArray<FMeshMaterial>& Materials, const std::filesystem::path& InPath, const std::filesystem::path& OutParentPath, FMaterialImportResult& OutResult)
{
	OutResult.MaterialIDs.Empty();
	OutResult.OutputPaths.Empty();
	OutResult.Headers.Empty();

	try
	{
		TMap<std::filesystem::path, FGuid> TextureMap;
		for (const FMeshMaterial& Material : Materials)
		{
			FGuid& MaterialID = OutResult.MaterialIDs.Emplace();
			std::filesystem::path& MaterialOutputPath = OutResult.OutputPaths.Emplace();
			FAssetFileHeader& MaterialHeader = OutResult.Headers.Emplace();

			MaterialID = FGuid::NewGuid();

			MaterialOutputPath = OutParentPath / std::format("{}.uasset", Material.Name.CStr());
			MaterialOutputPath = MakeUniquePath(MaterialOutputPath);

			MaterialHeader.Version = 1;
			MaterialHeader.AssetType = EAssetType::Material;
			MaterialHeader.AssetID = MaterialID;

			FWindowsBinWriter FileWriter(MaterialOutputPath);

			FileWriter << MaterialHeader;

			FMaterialPayload Payload;
			Payload.AmbientColor = Material.AmbientColor;
			Payload.DiffuseColor = Material.DiffuseColor;
			Payload.SpecularColor = Material.SpecularColor;
			Payload.DiffuseTexture = GetOrImportTexture(Material.DiffuseTexturePath, OutParentPath, TextureMap);
			Payload.SpecularTexture = GetOrImportTexture(Material.SpecularTexturePath, OutParentPath, TextureMap);
			Payload.NormalTexture = GetOrImportTexture(Material.NormalTexturePath, OutParentPath, TextureMap);
			Payload.Opacity = Material.Opacity;

			FMaterialFileIO::Save(FileWriter, Payload);
		}
	}
	catch (const std::exception&)
	{
		UE_LOG_ERROR("Failed to import materials from %s", InPath.string().c_str());
		return false;
	}

	return true;
}

FGuid FMaterialImporter::GetOrImportTexture(const std::filesystem::path& InPath, const std::filesystem::path& OutParentPath, TMap<std::filesystem::path, FGuid>& TextureMap)
{
	if (InPath.empty())
	{
		return FGuid();
	}

	if (TextureMap.Contains(InPath))
	{
		return TextureMap[InPath];
	}
	else
	{
		std::filesystem::path DestTexturePath = OutParentPath / InPath.filename();
		DestTexturePath.replace_extension(".uasset");
		DestTexturePath = MakeUniquePath(DestTexturePath);

		FAssetFileHeader TextureHeader;
		if (FTexture2DImporter::Import(InPath, DestTexturePath, TextureHeader))
		{
			TextureMap.Add(InPath, TextureHeader.AssetID);
			return TextureHeader.AssetID;
		}
		else
		{
			UE_LOG_ERROR("Failed to import texture: %s", DestTexturePath.string().c_str());
			return FGuid();
		}
	}
}


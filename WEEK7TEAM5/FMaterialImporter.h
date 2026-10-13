#pragma once

#include "Core.h"
#include "FAsset.h"
#include "FObjInfo.h"
#include "FMeshDescription.h"
#include "TArray.h"
#include "TMap.h"
#include "FGuid.h"
#include <optional>
#include <filesystem>

struct FMaterialImportResult
{
	TArray<FGuid> MaterialIDs;
	TArray<std::filesystem::path> OutputPaths;
	TArray<FAssetFileHeader> Headers;
};

class FMaterialImporter
{
public:
	static bool ImportDefault(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead);
    static bool Import(const FMeshMaterial& Material, const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead);
	static bool Import(const TArray<FMeshMaterial>& Materials, const std::filesystem::path& InPath, const std::filesystem::path& OutParentPath, FMaterialImportResult& OutResult);

private:
	static FGuid GetOrImportTexture(const std::filesystem::path& InPath, const std::filesystem::path& OutParentPath, TMap<std::filesystem::path, FGuid>& TextureMap);
};
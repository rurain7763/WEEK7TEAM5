#pragma once
#include <filesystem>
#include "FAsset.h"
#include <optional>
#include "FObjInfo.h"

class FMaterialImporter
{
public:
	static bool ImportDefault(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead);
    static bool Import(const FObjMaterialInfo& MaterialInfo, const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead);
};
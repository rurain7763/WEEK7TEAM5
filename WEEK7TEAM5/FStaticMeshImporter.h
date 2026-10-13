#pragma once
#include <filesystem>
#include "FAsset.h"
#include "FGuid.h"
#include "Transform.h"
#include <optional>

class FStaticMeshImporter
{
public:
    // .obj -> .uasset 변환
    static bool Import(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead);
    static bool Import(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead, const FTransform& Transform, const FVector4& OverrideColor);
};
#pragma once

#include "Core.h"
#include "TArray.h"
#include "Vector.h"
#include "FGuid.h"
#include <filesystem>

using FVertexID = uint32;
using FPolygonGroupID = uint32;

struct FVertex
{
    FVector Position;
    FVector Normal;
    FVector4 Color;
    FVector2 UV;
	FVector4 Tangent;
};

struct FMeshMaterial
{
	FString Name;

	FVector AmbientColor = FVector(0.0f, 0.0f, 0.0f); // Ka
	FVector DiffuseColor = FVector(1.0f, 1.0f, 1.0f); // Kd
	FVector SpecularColor = FVector(0.0f, 0.0f, 0.0f); // Ks
	float Shininess = 0.0f; // Ns
	float Opacity = 1.0f;                             // d 또는 Tr

	std::filesystem::path DiffuseTexturePath;                   // map_Kd
	std::filesystem::path SpecularTexturePath;                  // map_Ks
	std::filesystem::path NormalTexturePath;                    // map_Bump
};

struct FMeshSection
{
    uint32 FirstIndex = 0;
    uint32 IndexCount = 0;
	int32 MaterialIndex = -1;
};


#pragma once

#include "Core.h"
#include "Vector.h"
#include "TArray.h"
#include <filesystem>

struct FObjVertexIndex
{
    int32 PositionIndex = -1;
    int32 UVIndex = -1;
    int32 NormalIndex = -1;
};

struct FObjFace
{
    TArray<FObjVertexIndex> Vertices;
    int32 MaterialIndex = -1;
};

struct FObjObject
{
	FString Name;
	TArray<int32> FaceIndices;
};

struct FObjMaterialInfo
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

struct FObjInfo
{
    TArray<FVector> Positions;  // v
    TArray<FVector2> UVs;  // vt
    TArray<FVector> Normals;  // vn
    TArray<FObjFace> Faces;  // f
    TArray<FObjObject> Objects;  // o
    TArray<FObjMaterialInfo> Materials;
};
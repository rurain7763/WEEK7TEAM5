#pragma once

#include "Core.h"
#include "TArray.h"
#include "Stb/stb_image.h"
#include "FArchive.h"
#include "Serializers.h"
#include "Object.h"
#include "FObjInfo.h"
#include "FMeshDescription.h"
#include <filesystem>

// Texture 전용 Payload, FileIO
struct FImagePayload
{
	int32 Width;
	int32 Height;
	int32 Channels;
	TArray<int8> ImageData;
};

class FImageFileIO
{
public:
	static bool Load(const std::filesystem::path& FilePath, FImagePayload& OutPayload)
	{
		stbi_uc* ImageDataPtr = stbi_load(FilePath.string().c_str(), &OutPayload.Width, &OutPayload.Height, &OutPayload.Channels, 4);
		if (!ImageDataPtr)
		{
			return false;
		}

		OutPayload.ImageData.SetNum(OutPayload.Width * OutPayload.Height * 4);
		std::memcpy(OutPayload.ImageData.Data(), ImageDataPtr, OutPayload.ImageData.Num());

		stbi_image_free(ImageDataPtr);

		return true;
	}

	static bool Load(FArchive& Ar, FImagePayload& OutPayload)
	{
		Ar << OutPayload.Width;
		Ar << OutPayload.Height;
		Ar << OutPayload.Channels;
		Ar << OutPayload.ImageData;

		return true;
	}

	static bool Save(FArchive& Ar, FImagePayload& InPayload)
	{
		Ar << InPayload.Width;
		Ar << InPayload.Height;
		Ar << InPayload.Channels;
		Ar << InPayload.ImageData;

		return true;
	}
};

struct FStaticMeshPayload
{
	TArray<FVertex> Vertices;
	TArray<uint32> Indices;
	TArray<FGuid> Materials;
	TArray<FMeshSection> Sections;
};

struct FStaticMeshSourcePayload
{
	TArray<FVertex> Vertices;
	TArray<uint32> Indices;
	TArray<FMeshMaterial> Materials;
	TArray<FMeshSection> Sections;
};

// StaicMesh 전용 IO
class FStaticMeshFileIO
{
public:
	static bool Load(FArchive& Ar, FStaticMeshPayload& OutPayload)
	{
		Ar << OutPayload.Vertices;
		Ar << OutPayload.Indices;
		Ar << OutPayload.Sections;
		Ar << OutPayload.Materials;

		return true;
	}

	static bool Load(const std::filesystem::path& FilePath, FStaticMeshSourcePayload& OutPayload)
	{
		const std::filesystem::path Extension = FilePath.extension();

		if (Extension == ".obj")
		{
			return LoadFromObj(FilePath, OutPayload);
		}

		return false;
	}

	static bool Save(FArchive& Ar, FStaticMeshPayload& InPayload)
	{
		Ar << InPayload.Vertices;
		Ar << InPayload.Indices;
		Ar << InPayload.Sections;
		Ar << InPayload.Materials;

		return true;
	}

private:
	static bool LoadFromObj(const std::filesystem::path& FilePath, FStaticMeshSourcePayload& OutPayload);
	static bool ParseObjFaceVertex(const FString& Token, const FObjInfo& ObjInfo, FObjVertexIndex& OutIndex);
	static bool ParseObjMaterial(const std::filesystem::path& ParentPath, const std::filesystem::path& MaterialFilePath, FObjInfo& OutInfo);
	static bool ConvertStaticMeshSourcePayload(FObjInfo& ObjInfo, FStaticMeshSourcePayload& OutPayload);
};

// Material 전용 Payload, IO
struct FMaterialPayload
{
	FVector AmbientColor;
	FVector DiffuseColor;
	FVector SpecularColor;
	FGuid DiffuseTexture;
	FGuid SpecularTexture ;
	FGuid NormalTexture;
	float Shininess = 32.0f;
	float Opacity = 1.0f;
};

class FMaterialFileIO
{
public:
	static bool Load(FArchive& Ar, FMaterialPayload& OutPayload)
	{
		Ar << OutPayload.AmbientColor;
		Ar << OutPayload.DiffuseColor;
		Ar << OutPayload.SpecularColor;
		Ar << OutPayload.DiffuseTexture;
		Ar << OutPayload.SpecularTexture;
		Ar << OutPayload.NormalTexture;
		Ar << OutPayload.Shininess;
		Ar << OutPayload.Opacity;
		
		return true;
	}
	
	// mtl -> uasset
	static bool Save(FArchive& Ar, FMaterialPayload& InPayload)
	{
		Ar << InPayload.AmbientColor;
		Ar << InPayload.DiffuseColor;
		Ar << InPayload.SpecularColor;
		Ar << InPayload.DiffuseTexture;
		Ar << InPayload.SpecularTexture;
		Ar << InPayload.NormalTexture;
		Ar << InPayload.Shininess;
		Ar << InPayload.Opacity;
		
		return true;
	}
};

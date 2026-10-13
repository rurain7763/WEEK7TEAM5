#include "FStaticMeshImporter.h"
#include "FArchive.h"
#include "FLogManager.h"
#include "FGuid.h"
#include "Serializers.h"
#include "FMaterialImporter.h"
#include "AssetFileIOs.h"
#include "Matrix.h"
#include "FTexture2DImporter.h"
#include "FileManager.h"

// .obj -> .uasset 변환
bool FStaticMeshImporter::Import(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
    FStaticMeshSourcePayload SourcePayload;
    if (!FStaticMeshFileIO::Load(InPath, SourcePayload))
    {
        UE_LOG_ERROR("Failed to load static mesh source: %s", InPath.string().c_str());
        return false;
    }

    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::StaticMesh;
    OutHead.AssetID = FGuid::NewGuid();

    try
    {
        FStaticMeshPayload Payload;
		Payload.Vertices = SourcePayload.Vertices;
		Payload.Indices = SourcePayload.Indices;
		Payload.Sections = SourcePayload.Sections;

		FMaterialImportResult MaterialsResult;
		if (!FMaterialImporter::Import(SourcePayload.Materials, InPath, OutPath.parent_path(), MaterialsResult))
		{
			UE_LOG_ERROR("Failed to import materials from %s", InPath.string().c_str());
			return false;
		}

		for (int32 i = 0; i < SourcePayload.Materials.Num(); ++i)
		{
			Payload.Materials.Add(MaterialsResult.MaterialIDs[i]);
		}

        FWindowsBinWriter FileWriter(OutPath);
        
        FileWriter << OutHead;

        FStaticMeshFileIO::Save(FileWriter, Payload);
    }
    catch(const std::exception&)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }
    
    return true;
}   

bool FStaticMeshImporter::Import(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead, const FTransform& Transform, const FVector4& OverrideColor)
{
    FStaticMeshSourcePayload SourcePayload;
    if (!FStaticMeshFileIO::Load(InPath, SourcePayload))
    {
        UE_LOG_ERROR("Failed to load static mesh source: %s", InPath.string().c_str());
        return false;
    }

    const FMatrix BakeMatrix = Transform.MakeMatrix();
    const FMatrix NormalMatrix = BakeMatrix.AffineInverse().Transpose();
    for (FVertex& Vertex : SourcePayload.Vertices)
    {
        Vertex.Position = BakeMatrix.TransformPosition(Vertex.Position);
        Vertex.Normal = NormalMatrix.TransformVector(Vertex.Normal);
        Vertex.Normal.Normalize();
		Vertex.Color = OverrideColor;
    }

    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::StaticMesh;
    OutHead.AssetID = FGuid::NewGuid();

    try
    {
        FStaticMeshPayload Payload;
        Payload.Vertices = SourcePayload.Vertices;
        Payload.Indices = SourcePayload.Indices;
        Payload.Sections = SourcePayload.Sections;

        FMaterialImportResult MaterialsResult;
        if (!FMaterialImporter::Import(SourcePayload.Materials, InPath, OutPath.parent_path(), MaterialsResult))
        {
            UE_LOG_ERROR("Failed to import materials from %s", InPath.string().c_str());
            return false;
        }

        for (int32 i = 0; i < SourcePayload.Materials.Num(); ++i)
        {
            Payload.Materials.Add(MaterialsResult.MaterialIDs[i]);
        }

        FWindowsBinWriter FileWriter(OutPath);

        FileWriter << OutHead;

        FStaticMeshFileIO::Save(FileWriter, Payload);
    }
    catch (const std::exception&)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }

    return true;
}




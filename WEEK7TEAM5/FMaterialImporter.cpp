#include "FMaterialImporter.h"
#include "AssetFileIOs.h"
#include "FArchive.h"
#include "FLogManager.h"
#include "FGuid.h"
#include "Serializers.h"
#include "FTexture2DImporter.h"

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

bool FMaterialImporter::Import(const FObjMaterialInfo& MaterialInfo, const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::Material;
    OutHead.AssetID = FGuid::NewGuid();

    try
    {
        FGuid DiffuseTextureID;

        if (MaterialInfo.DiffuseTexturePath.Len() != 0)
        {
            // 1차 검색 : Textures 폴더에서 찾기
           // std::filesystem::path TexturePath = InPath.parent_path() / "Textures" / MaterialInfo.DiffuseTexturePath.CStr();
            const std::filesystem::path TexturePath = (InPath / MaterialInfo.DiffuseTexturePath.CStr()).lexically_normal().generic_string();

            // InPath는 원본 OBJ의 폴더
            // 텍스처 원본은 여기서 읽고, uasset은 Material과 같은 프로젝트 Assets 폴더에 저장
            std::optional<std::filesystem::path> TextureUAssetPath = FTexture2DImporter::GetorImport(TexturePath, OutPath.parent_path());
            
            // 2차 검색 : mtl과 같은 폴더 (Mashes)에서 찾기
            if (!TextureUAssetPath)
            {
                const std::filesystem::path FallPath = InPath / MaterialInfo.DiffuseTexturePath.CStr();
                TextureUAssetPath = FTexture2DImporter::GetorImport(FallPath, OutPath.parent_path());
                
            }
            
            // 그래도 없으면 에러 로그
            if (!TextureUAssetPath)
            {
                UE_LOG_ERROR("Failed to find Diffuse Texture: %s", TexturePath.string().c_str());
                return false;

            }
            
            FWindowsBinReader Reader(*TextureUAssetPath);
            FAssetFileHeader TextureHeader;
            Reader << TextureHeader;
            DiffuseTextureID = TextureHeader.AssetID;
            
        }   
        
        FVector ZeroVector(0.0f);
        FGuid EmptyGuid;
        FWindowsBinWriter FileWriter(OutPath);

        FVector DiffuseColor = MaterialInfo.DiffuseColor;  // const 떼어내기
        float Opacity = MaterialInfo.Opacity;
        	
        FMaterialPayload Payload;

        Payload.AmbientColor = DiffuseColor;
        Payload.DiffuseColor = DiffuseColor;
        Payload.SpecularColor = ZeroVector;
        Payload.DiffuseTexture = DiffuseTextureID;
        Payload.SpecularTexture = EmptyGuid;
        Payload.NormalTexture = EmptyGuid;
        Payload.Opacity = Opacity;

        FileWriter << OutHead;
        FMaterialFileIO::Save(FileWriter, Payload);
    }
    catch(const std::exception&)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }

    return true;
}

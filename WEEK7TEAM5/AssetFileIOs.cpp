#include "AssetFileIOs.h"
#include "FLogManager.h"
#include "EngineMathLibrary.h"
#include <fstream>
#include <sstream>

bool FStaticMeshFileIO::LoadFromObj(const std::filesystem::path& FilePath, FStaticMeshSourcePayload& OutPayload)
{
	std::ifstream File(FilePath);

	if (!File.is_open())
	{
		UE_LOG_ERROR("Failed to convert OBJ mesh");
		return false;
	}

	std::filesystem::path ParentPath = FilePath.parent_path();

	FObjInfo ObjInfo;
    
    FObjObject& DefaultObject = ObjInfo.Objects.Emplace();
	DefaultObject.Name = FString("Default");

	TMap<std::string, int32> ObjectNameToIndex;
	ObjectNameToIndex.Add(DefaultObject.Name, 0);

	int32 CurrentObjectIndex = 0;
	int32 CurrentMaterialIndex = -1;

	std::string Line;
	while (std::getline(File, Line))
	{
		std::istringstream Stream(Line);
		std::string Command;
		Stream >> Command;

		if (Command == "v")
		{
			float X, Y, Z;
			Stream >> X >> Y >> Z;
			ObjInfo.Positions.Add(FVector(X, Y, Z));
		}
		else if (Command == "vt")
		{
			float U, V;
			Stream >> U >> V;

			// .obj vt는 관례상 v=0이 텍스처 하단이나,
			// directx는 v=0이 텍스처 상단
			ObjInfo.UVs.Add(FVector2(U, 1.0f - V));
		}
		else if (Command == "vn")
		{
			float X, Y, Z;
			Stream >> X >> Y >> Z;
			ObjInfo.Normals.Add(FVector(X, Y, Z));
		}
		else if (Command == "f")
		{
			int32 FaceIndex = ObjInfo.Faces.Num();

			FObjFace& Face = ObjInfo.Faces.Emplace();
			Face.MaterialIndex = CurrentMaterialIndex;

			std::string Token;
			while (Stream >> Token)
			{
				FObjVertexIndex VertexIndex;

				if (!ParseObjFaceVertex(Token, ObjInfo, VertexIndex))
				{
					return false;

					UE_LOG_ERROR("Failed to convert OBJ mesh");
				}

				Face.Vertices.Add(VertexIndex);
			}

			if (Face.Vertices.Num() < 3)
			{
				UE_LOG_ERROR("Failed to convert OBJ mesh");
				return false;
			}

			if (CurrentObjectIndex != -1)
			{
				ObjInfo.Objects[CurrentObjectIndex].FaceIndices.Add(FaceIndex);
			}
		}
		else if (Command == "o")
		{
			std::string ObjectName;
			Stream >> ObjectName;

			if (!ObjectNameToIndex.Contains(ObjectName))
			{
				CurrentObjectIndex = ObjInfo.Objects.Num();

				FObjObject& NewObject = ObjInfo.Objects.Emplace();
				NewObject.Name = ObjectName;

				ObjectNameToIndex.Add(ObjectName, CurrentObjectIndex);
			}
			else
			{
				CurrentObjectIndex = ObjectNameToIndex[ObjectName];
			}
		}
		else if (Command == "usemtl")
		{
			std::string MaterialName;

			Stream >> MaterialName;

			for (int32 MaterialIndex = 0; MaterialIndex < ObjInfo.Materials.Num(); ++MaterialIndex)
			{
				if (ObjInfo.Materials[MaterialIndex].Name == FString(MaterialName))
				{
					CurrentMaterialIndex = MaterialIndex;

					break;
				}
			}
		}
		else if (Command == "mtllib")
		{
			std::string MtlFileName;
			Stream >> MtlFileName;

            ParseObjMaterial(ParentPath, ParentPath / MtlFileName, ObjInfo);
		}
	}

    return ConvertStaticMeshSourcePayload(ObjInfo, OutPayload);
}

bool FStaticMeshFileIO::ParseObjFaceVertex(const FString& Token, const FObjInfo& ObjInfo, FObjVertexIndex& OutIndex)
{
    const std::string Value = Token.CStr();
    const size_t FirstSlash = Value.find('/');
    const size_t SecondSlash = FirstSlash == std::string::npos ? std::string::npos : Value.find('/', FirstSlash + 1);

    auto ParseIndex = [](const std::string& Text, int32 ElementCount, int32& OutValue) -> bool
        {
            if (Text.empty())
            {
                OutValue = -1;
                return true;
            }

            try
            {
                size_t ParsedCharacters = 0;
                const int32 RawIndex = std::stoi(Text, &ParsedCharacters);
                if (ParsedCharacters != Text.size() || RawIndex == 0)
                {
                    return false;
                }

                OutValue = RawIndex > 0 ? RawIndex - 1 : ElementCount + RawIndex;
                return OutValue >= 0 && OutValue < ElementCount;
            }
            catch (const std::exception&)
            {
                return false;
            }
        };

    const std::string PositionText = Value.substr(0, FirstSlash);
    const std::string UVText = FirstSlash == std::string::npos
        ? ""
        : Value.substr(FirstSlash + 1, (SecondSlash == std::string::npos ? Value.size() : SecondSlash) - FirstSlash - 1);
    const std::string NormalText = SecondSlash == std::string::npos ? "" : Value.substr(SecondSlash + 1);

    // UE_LOG_ERROR("UV : %f %f", ObjInfo.UVs[0].X, ObjInfo.UVs[0].Y);

    return ParseIndex(PositionText, ObjInfo.Positions.Num(), OutIndex.PositionIndex)
        && ParseIndex(UVText, ObjInfo.UVs.Num(), OutIndex.UVIndex)
        && ParseIndex(NormalText, ObjInfo.Normals.Num(), OutIndex.NormalIndex);
}

bool FStaticMeshFileIO::ParseObjMaterial(const std::filesystem::path& ParentPath, const std::filesystem::path& MaterialFilePath, FObjInfo& OutInfo)
{
    std::ifstream File(MaterialFilePath);
    if (!File.is_open())
    {
        return false;
    }

    FObjMaterialInfo* CurrentMaterial = nullptr;
    std::string Line;

    while (std::getline(File, Line))
    {
        std::istringstream Stream(Line);
        std::string Command;
        Stream >> Command;

        if (Command.empty())
        {
            continue;
        }

        if (Command[0] == '#')
        {
            continue;
        }

        if (Command == "newmtl")
        {
            std::string MaterialName;
            Stream >> MaterialName;
            if (MaterialName.empty())
            {
                CurrentMaterial = nullptr;

                continue;
            }

            FObjMaterialInfo NewMaterial;
            NewMaterial.Name = FString(MaterialName);
            OutInfo.Materials.Add(NewMaterial);

            CurrentMaterial = &OutInfo.Materials[OutInfo.Materials.Num() - 1];
            continue;
        }
        if (!CurrentMaterial)
        {
            continue;
        }

		if (Command == "Ka")
		{
			float R = 0.0f;
			float G = 0.0f;
			float B = 0.0f;

			if (Stream >> R >> G >> B)
			{
				CurrentMaterial->AmbientColor = FVector(R, G, B);
			}
		}
        else if (Command == "Kd")
        {
            float R = 1.0f;
            float G = 1.0f;
            float B = 1.0f;
            if (Stream >> R >> G >> B)
            {
                CurrentMaterial->DiffuseColor = FVector(R, G, B);
            }
        }
		else if (Command == "Ks")
		{
			float R = 0.0f;
			float G = 0.0f;
			float B = 0.0f;

			if (Stream >> R >> G >> B)
			{
				CurrentMaterial->SpecularColor = FVector(R, G, B);
			}
		}
		else if (Command == "Ns")
		{
			float Shininess = 0.0f;
			if (Stream >> Shininess)
			{
				CurrentMaterial->Shininess = Shininess;
			}
		}
        else if (Command == "d")
        {
            float Opacity = 1.0f;
            if (Stream >> Opacity)
            {
                CurrentMaterial->Opacity = Opacity;
            }
        }
        else if (Command == "Tr")
        {
            float Transparency = 0.0f;
            if (Stream >> Transparency)
            {
                CurrentMaterial->Opacity = 1.0f - Transparency;
            }
        }
        else if (Command == "map_Kd")
        {
            std::string TexturePath;
            Stream >> TexturePath;

			CurrentMaterial->DiffuseTexturePath = ParentPath / TexturePath;
        }
        else if (Command == "map_Ks")
        {
			std::string TexturePath;
			Stream >> TexturePath;

			CurrentMaterial->SpecularTexturePath = ParentPath / TexturePath;
        }
		else if (Command == "norm")
		{
			std::string TexturePath;
			Stream >> TexturePath;

			CurrentMaterial->NormalTexturePath = ParentPath / TexturePath;
		}
    }

    return true;
}

bool FStaticMeshFileIO::ConvertStaticMeshSourcePayload(FObjInfo& ObjInfo, FStaticMeshSourcePayload& OutPayload)
{
	OutPayload.Materials.SetNum(ObjInfo.Materials.Num());
	for (int32 i = 0; i < ObjInfo.Materials.Num(); ++i)
	{
		const FObjMaterialInfo& ObjMaterial = ObjInfo.Materials[i];

		FMeshMaterial& MeshMaterial = OutPayload.Materials[i];
		MeshMaterial.Name = ObjMaterial.Name;
		MeshMaterial.AmbientColor = ObjMaterial.AmbientColor;
		MeshMaterial.DiffuseColor = ObjMaterial.DiffuseColor;
		MeshMaterial.SpecularColor = ObjMaterial.SpecularColor;
		MeshMaterial.DiffuseTexturePath = ObjMaterial.DiffuseTexturePath;
		MeshMaterial.SpecularTexturePath = ObjMaterial.SpecularTexturePath;
		MeshMaterial.NormalTexturePath = ObjMaterial.NormalTexturePath;
		MeshMaterial.Shininess = ObjMaterial.Shininess;
		MeshMaterial.Opacity = ObjMaterial.Opacity;
	}

    OutPayload.Vertices.Empty();
	OutPayload.Indices.Empty();
	OutPayload.Sections.Empty();

	for (FObjObject& Obj : ObjInfo.Objects)
	{
		// NOTE: 재질별로 정렬하여 섹션을 나누기 위해 정렬
		std::sort(Obj.FaceIndices.begin(), Obj.FaceIndices.end(), [&](int32 A, int32 B) {
			const FObjFace& FaceA = ObjInfo.Faces[A];
			const FObjFace& FaceB = ObjInfo.Faces[B];
			return FaceA.MaterialIndex < FaceB.MaterialIndex;
		});

        FMeshSection* CurrentSection = nullptr;
        for (int32 FaceIndex : Obj.FaceIndices)
        {
			const FObjFace& Face = ObjInfo.Faces[FaceIndex];

			// NOTE: 재질이 바뀌면 새로운 섹션을 생성
			if (!CurrentSection || Face.MaterialIndex != CurrentSection->MaterialIndex)
			{
				CurrentSection = &OutPayload.Sections.Emplace();
				CurrentSection->FirstIndex = OutPayload.Indices.Num();
				CurrentSection->MaterialIndex = Face.MaterialIndex;
			}

            const int32 FaceVertexCount = Face.Vertices.Num();
            if (FaceVertexCount < 3)
            {
                return false;
            }

            // NOTE: 삼각분할
            int32 TriangleCount = FaceVertexCount - 2;
            TArray<FIntVector3> Triangles;
            Triangles.SetNum(TriangleCount);
            for (int32 i = 0; i < TriangleCount; ++i)
            {
                FIntVector3& Triangle = Triangles[i];
                Triangle.X = 0;
                Triangle.Y = i + 1;
                Triangle.Z = i + 2;
            }

            for (const FIntVector3& Triangle : Triangles)
            {
                const FObjVertexIndex* Indices[] = {
                    &Face.Vertices[Triangle.X],
                    &Face.Vertices[Triangle.Y],
                    &Face.Vertices[Triangle.Z]
                };

                for (int32 i = 0; i < 3; i++)
                {
                    const FObjVertexIndex* ObjIndex = Indices[i];

					int32 VertexIndex = OutPayload.Vertices.Num();

                    FVertex& Vertex = OutPayload.Vertices.Emplace();
                    Vertex.Position = ObjInfo.Positions[ObjIndex->PositionIndex];

                    if (ObjIndex->UVIndex >= 0)
                    {
                        if (ObjIndex->UVIndex >= ObjInfo.UVs.Num())
                        {
                            return false;
                        }

                        Vertex.UV = ObjInfo.UVs[ObjIndex->UVIndex];
                    }
                    else
                    {
                        Vertex.UV = FVector2(0, 0);
                    }

                    if (ObjIndex->NormalIndex >= 0)
                    {
                        if (ObjIndex->NormalIndex >= ObjInfo.Normals.Num())
                        {
                            return false;
                        }

                        Vertex.Normal = ObjInfo.Normals[ObjIndex->NormalIndex];
                    }
                    else
                    {
                        FVector P0 = ObjInfo.Positions[Face.Vertices[Triangle.X].PositionIndex];
                        FVector P1 = ObjInfo.Positions[Face.Vertices[Triangle.Y].PositionIndex];
                        FVector P2 = ObjInfo.Positions[Face.Vertices[Triangle.Z].PositionIndex];

                        Vertex.Normal = CalculateNormal(P0, P1, P2);
                    }

                    Vertex.Color = FVector4(1, 1, 1, 1);

                    OutPayload.Indices.Add(VertexIndex);

					CurrentSection->IndexCount++;
                }
            }
        }
	}

	CalculateTangents(OutPayload.Vertices.Data(), OutPayload.Vertices.Num(), OutPayload.Indices.Data(), OutPayload.Indices.Num());

    return true;
}

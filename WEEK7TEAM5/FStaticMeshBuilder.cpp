#include "FStaticMeshBuilder.h"
#include <cassert>
#include "MathUtility.h"

bool FStaticMeshBuilder::Build (const FMeshDescription& MeshDescription,
    FStaticMeshBuildData& OutCookedData)
{
    OutCookedData.Vertices.Empty();
    OutCookedData.Indices.Empty();
    OutCookedData.Sections.Empty();

    for (uint32 PolygonGroupIndex = 0; 
        PolygonGroupIndex < static_cast<uint32>(MeshDescription.PolygonGroups.Num());
        ++PolygonGroupIndex)
    {
        FStaticMeshSection Section;

        Section.FirstIndex = static_cast<uint32>(OutCookedData.Indices.Num());
        Section.MaterialName =  MeshDescription.PolygonGroups[PolygonGroupIndex].MaterialName;

        for (const FMeshTriangle& Triangle : MeshDescription.Triangles)
        {
            if (Triangle.PolygonGroupID != PolygonGroupIndex)
            {
                continue;
            }

            for (uint32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
            {
                const FVertexInstanceID VertexInstanceID = Triangle.Corners[CornerIndex];

                if (VertexInstanceID >= static_cast<uint32>(MeshDescription.VertexInstances.Num()))
                {
                    return false;
                }

                const FMeshVertexInstance& VertexInstance = MeshDescription.VertexInstances[VertexInstanceID];
                const FVertexID VertexID = VertexInstance.VertexID;

                if (VertexID >= static_cast<uint32>(MeshDescription.Vertices.Num()))
                {
                    return false;
                }

                const FMeshVertexPosition& MeshVertex =  MeshDescription.Vertices[VertexID];

                FVertex RenderVertex;
                RenderVertex.Pos = MeshVertex.Position;
                RenderVertex.Normal = VertexInstance.Normal;
                RenderVertex.Color = VertexInstance.Color;
                RenderVertex.Tex = VertexInstance.TexCoord;

                const uint32 NewVertexIndex = static_cast<uint32>(OutCookedData.Vertices.Num());

                OutCookedData.Vertices.Add(RenderVertex);
                OutCookedData.Indices.Add(NewVertexIndex);
            }
        }

        Section.IndexCount = static_cast<uint32>(OutCookedData.Indices.Num()) - Section.FirstIndex;
		if (Section.IndexCount > 0)
		{
			OutCookedData.Sections.Add(Section);
		}
	}

	return true;
}

bool FStaticMeshBuilder::HasTangents(const TArray<FVertex>& Vertices)
{
    for (const FVertex& Vertex : Vertices)
    {
        if (Vertex.Tangent.LengthSquared() > SMALL_NUMBER)
        {
            return true;
        }
    }
    return false;
}

void FStaticMeshBuilder::CalculateTangents(TArray<FVertex>& Vertices, const TArray<uint32>& Indices)
{
	const int32 VertexCount = Vertices.Num();
	const int32 IndexCount = Indices.Num();

	TArray<FVector> TangentSums;
	TangentSums.Init(FVector(0.0f), VertexCount);

	TArray<FVector> BitangentSums;
	BitangentSums.Init(FVector(0.0f), VertexCount);

	//T, B구하기
	for (int32 Index = 0; Index + 2 < IndexCount; Index += 3)
	{
		const int32 Index0 = Indices[Index];
		const int32 Index1 = Indices[Index + 1];
		const int32 Index2 = Indices[Index + 2];

		if (Index0 >= VertexCount || Index1 >= VertexCount || Index2 >= VertexCount)
		{
			continue;
		}

		const FVertex& V0 = Vertices[Index0];
		const FVertex& V1 = Vertices[Index1];
		const FVertex& V2 = Vertices[Index2];


		//p1 - p0
		const FVector E1 = V1.Pos - V0.Pos;
		//p2 - p0
		const FVector E2 = V2.Pos - V0.Pos;
		//u1 - u0
		const float Du1 = V1.Tex.X - V0.Tex.X;
		//v1 - v0
		const float Dv1 = V1.Tex.Y - V0.Tex.Y;
		//u2 - u0
		const float Du2 = V2.Tex.X - V0.Tex.X;
		//v2 - v0
		const float Dv2 = V2.Tex.Y - V0.Tex.Y;

		// 2×2 행렬식
		const float R = Du1 * Dv2 - Dv1 * Du2;

		// UV가 퇴화한 삼각형
		if (FMath::Abs(R) < 1e-10f) continue;


		const float InvR = 1.0f / R;

		const FVector FaceT = (E1 * Dv2 - E2 * Dv1) * InvR;
		const FVector FaceB = (E2 * Du1 - E1 * Du2) * InvR;

		TangentSums[Index0] += FaceT;
		TangentSums[Index1] += FaceT;
		TangentSums[Index2] += FaceT;

		BitangentSums[Index0] += FaceB;
		BitangentSums[Index1] += FaceB;
		BitangentSums[Index2] += FaceB;
	}

	//정점을 공유하는 여러 삼각형이 있을 수 있으므로 합산을 구한 후 계산
	for (int32 i = 0; i < VertexCount; ++i)
	{
		FVertex& Vertex = Vertices[i];
		const FVector& N = Vertex.Normal;

		// Gram-Schmidt: T에서 N 성분을 빼서 N에 수직으로
		FVector T = TangentSums[i] - N * FVector::dot(N, TangentSums[i]);

		float W = 1.0f;
		if (T.LengthSquared() > 1e-10f)
		{
			T.Normalize();
			// 실제 B가 cross(N, T)와 같은 쪽이면 +1, 반대면 -1
			W = (FVector::dot(FVector::cross(N, T), BitangentSums[i]) < 0.0f) ? -1.0f : 1.0f;
		}
		else
		{
			// 대체 경로: UV가 없거나 퇴화한 정점. 임시로 만들기
			const FVector Axis = (FMath::Abs(N.z) < 0.999f) ? FVector(0, 0, 1) : FVector(1, 0, 0);
			T = Axis - N * FVector::dot(Axis, N);
			T.Normalize();
			W = 1.0f;
		}

		Vertex.Tangent = FVector4(T, W);
	}
}


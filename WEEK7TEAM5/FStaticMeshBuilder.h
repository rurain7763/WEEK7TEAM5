#pragma once

#include "FMeshDescription.h"

class FStaticMeshBuilder
{
public:
	static bool Build (const FMeshDescription& MeshDescription,
		FStaticMeshBuildData& OutCookedData);

	static bool HasTangents(const TArray<FVertex>& Vertices);
	static void CalculateTangents(TArray<FVertex>& Vertices, const TArray<uint32>& Indices);
};
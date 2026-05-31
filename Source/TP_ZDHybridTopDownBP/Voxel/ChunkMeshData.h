#pragma once

#include "CoreMinimal.h"

// CPU-side mesh buffers fed to UProceduralMeshComponent::CreateMeshSection_LinearColor.
struct FTPChunkMeshData
{
	TArray<FVector>      Vertices;
	TArray<int32>        Triangles;
	TArray<FVector>      Normals;
	TArray<FVector2D>    UV0;
	TArray<FLinearColor> Colors;

	void Reset()
	{
		Vertices.Reset();
		Triangles.Reset();
		Normals.Reset();
		UV0.Reset();
		Colors.Reset();
	}
};

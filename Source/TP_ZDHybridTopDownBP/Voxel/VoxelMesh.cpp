#include "VoxelMesh.h"
#include "Chunk.h"
#include "ChunkMeshData.h"
#include "BlockType.h"
#include "VoxelAtlas.h"

namespace
{
	// Face order: 0:+X 1:-X 2:+Y 3:-Y 4:+Z 5:-Z
	const FVector FaceNormal[6] = {
		FVector( 1,  0,  0), FVector(-1,  0,  0),
		FVector( 0,  1,  0), FVector( 0, -1,  0),
		FVector( 0,  0,  1), FVector( 0,  0, -1)
	};

	const FIntVector FaceDir[6] = {
		FIntVector( 1,  0,  0), FIntVector(-1,  0,  0),
		FIntVector( 0,  1,  0), FIntVector( 0, -1,  0),
		FIntVector( 0,  0,  1), FIntVector( 0,  0, -1)
	};

	// 4 corners per face on the unit cube, ordered CCW as seen from outside.
	const FVector FaceVerts[6][4] = {
		// +X
		{ FVector(1, 0, 0), FVector(1, 1, 0), FVector(1, 1, 1), FVector(1, 0, 1) },
		// -X
		{ FVector(0, 1, 0), FVector(0, 0, 0), FVector(0, 0, 1), FVector(0, 1, 1) },
		// +Y
		{ FVector(1, 1, 0), FVector(0, 1, 0), FVector(0, 1, 1), FVector(1, 1, 1) },
		// -Y
		{ FVector(0, 0, 0), FVector(1, 0, 0), FVector(1, 0, 1), FVector(0, 0, 1) },
		// +Z
		{ FVector(0, 0, 1), FVector(1, 0, 1), FVector(1, 1, 1), FVector(0, 1, 1) },
		// -Z
		{ FVector(0, 1, 0), FVector(1, 1, 0), FVector(1, 0, 0), FVector(0, 0, 0) }
	};

	// Sampler returns the block id for any local coord in [-1, ChunkSize]
	// (the chunk's own cells plus the 1-block apron used for face culling).
	void BuildImpl(FTPChunkMeshData& Out, const TFunctionRef<BlockId(int32, int32, int32)>& Sample)
	{
		using namespace VoxelConst;
		Out.Reset();

		for (int32 z = 0; z < ChunkSize; ++z)
		for (int32 y = 0; y < ChunkSize; ++y)
		for (int32 x = 0; x < ChunkSize; ++x)
		{
			const FTPBlockType& Type = FTPBlockRegistry::Get(static_cast<ETPBlockId>(Sample(x, y, z)));
			if (!Type.bSolid)
			{
				continue;
			}

			for (int32 f = 0; f < 6; ++f)
			{
				const FIntVector N = FaceDir[f];
				const FTPBlockType& NT = FTPBlockRegistry::Get(
					static_cast<ETPBlockId>(Sample(x + N.X, y + N.Y, z + N.Z)));

				const bool bNeighborTransparent = !NT.bSolid || NT.bTransparent;
				if (!bNeighborTransparent)
				{
					continue;
				}

				const int32 Base = Out.Vertices.Num();
				const FVector Origin(x * BlockSize, y * BlockSize, z * BlockSize);
				const FLinearColor Col(Type.Color);

				for (int32 v = 0; v < 4; ++v)
				{
					Out.Vertices.Add(Origin + FaceVerts[f][v] * BlockSize);
					Out.Normals.Add(FaceNormal[f]);
					Out.Colors.Add(Col);
				}

				FVector2D FaceUV[4];
				VoxelAtlas::TileUV(Type.AtlasIndex[f], FaceUV);
				Out.UV0.Add(FaceUV[0]);
				Out.UV0.Add(FaceUV[1]);
				Out.UV0.Add(FaceUV[2]);
				Out.UV0.Add(FaceUV[3]);

				// FaceVerts are CCW seen from outside; UE treats CW-from-front as
				// front-facing, so emit reversed winding for correct outward faces.
				Out.Triangles.Add(Base + 0); Out.Triangles.Add(Base + 2); Out.Triangles.Add(Base + 1);
				Out.Triangles.Add(Base + 0); Out.Triangles.Add(Base + 3); Out.Triangles.Add(Base + 2);
			}
		}
	}

	FORCEINLINE bool InRange(int32 V)
	{
		return V >= 0 && V < VoxelConst::ChunkSize;
	}
}

void VoxelMesh::BuildChunkMesh(const FTPChunk& C, FTPChunkMeshData& Out)
{
	BuildImpl(Out, [&C](int32 X, int32 Y, int32 Z) -> BlockId
	{
		if (InRange(X) && InRange(Y) && InRange(Z))
		{
			return C.Get(X, Y, Z);
		}
		return static_cast<BlockId>(ETPBlockId::Air); // border = exposed
	});
}

void VoxelMesh::BuildChunkMeshPadded(const TArray<BlockId>& Padded, FTPChunkMeshData& Out)
{
	BuildImpl(Out, [&Padded](int32 X, int32 Y, int32 Z) -> BlockId
	{
		return Padded[PaddedIndex(X, Y, Z)];
	});
}

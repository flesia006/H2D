#include "VoxelMesh.h"
#include "Chunk.h"
#include "ChunkMeshData.h"
#include "BlockType.h"

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

	// If faces render inside-out, flip this to swap triangle winding.
	// (Phase 1 material is set Two Sided, so this is rarely needed.)
	constexpr bool bFlipWinding = false;
}

void VoxelMesh::BuildChunkMesh(const FTPChunk& C, FTPChunkMeshData& Out)
{
	using namespace VoxelConst;
	Out.Reset();

	for (int32 z = 0; z < ChunkSize; ++z)
	for (int32 y = 0; y < ChunkSize; ++y)
	for (int32 x = 0; x < ChunkSize; ++x)
	{
		const FTPBlockType& Type = FTPBlockRegistry::Get(static_cast<ETPBlockId>(C.Get(x, y, z)));
		if (!Type.bSolid)
		{
			continue;
		}

		for (int32 f = 0; f < 6; ++f)
		{
			const FIntVector N = FaceDir[f];
			const int32 nx = x + N.X, ny = y + N.Y, nz = z + N.Z;

			bool bNeighborTransparent;
			if (nx < 0 || ny < 0 || nz < 0 || nx >= ChunkSize || ny >= ChunkSize || nz >= ChunkSize)
			{
				bNeighborTransparent = true; // chunk border (Phase 2: query neighbor chunk)
			}
			else
			{
				const FTPBlockType& NT = FTPBlockRegistry::Get(static_cast<ETPBlockId>(C.Get(nx, ny, nz)));
				bNeighborTransparent = !NT.bSolid || NT.bTransparent;
			}

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

			Out.UV0.Add(FVector2D(0, 0));
			Out.UV0.Add(FVector2D(1, 0));
			Out.UV0.Add(FVector2D(1, 1));
			Out.UV0.Add(FVector2D(0, 1));

			if (!bFlipWinding)
			{
				Out.Triangles.Add(Base + 0); Out.Triangles.Add(Base + 1); Out.Triangles.Add(Base + 2);
				Out.Triangles.Add(Base + 0); Out.Triangles.Add(Base + 2); Out.Triangles.Add(Base + 3);
			}
			else
			{
				Out.Triangles.Add(Base + 0); Out.Triangles.Add(Base + 2); Out.Triangles.Add(Base + 1);
				Out.Triangles.Add(Base + 0); Out.Triangles.Add(Base + 3); Out.Triangles.Add(Base + 2);
			}
		}
	}
}

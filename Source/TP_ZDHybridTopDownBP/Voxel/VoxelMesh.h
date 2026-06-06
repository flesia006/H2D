#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.h"
#include "Templates/Function.h"

struct FTPChunk;
struct FTPChunkMeshData;

namespace VoxelMesh
{
	// Padded volume = chunk + a 1-block apron on every face.
	constexpr int32 PaddedSize   = VoxelConst::ChunkSize + 2;                 // 18
	constexpr int32 PaddedVolume = PaddedSize * PaddedSize * PaddedSize;      // 5832

	// Maps local coords in [-1, ChunkSize] onto the padded array.
	FORCEINLINE int32 PaddedIndex(int32 X, int32 Y, int32 Z)
	{
		return (X + 1) + (Y + 1) * PaddedSize + (Z + 1) * PaddedSize * PaddedSize;
	}

	// Single chunk; border faces treated as exposed (Phase 1 standalone actor).
	void BuildChunkMesh(const FTPChunk& Chunk, FTPChunkMeshData& Out);

	// Mesh from a padded block volume (chunk + neighbor apron). Border faces cull
	// against the apron so chunks join seamlessly. ColumnTint (ChunkArea entries,
	// indexed x + y*ChunkSize) is emitted as vertex color on grass/leaves faces
	// for biome tinting; other blocks get white. Self-contained -> thread-safe.
	void BuildChunkMeshPadded(const TArray<BlockId>& Padded, const TArray<FColor>& ColumnTint,
		FTPChunkMeshData& Out);
}

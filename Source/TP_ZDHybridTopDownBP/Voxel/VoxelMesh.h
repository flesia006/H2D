#pragma once

#include "CoreMinimal.h"

struct FTPChunk;
struct FTPChunkMeshData;

namespace VoxelMesh
{
	// Builds a face-culled mesh for a single chunk.
	// Chunk-border faces are treated as exposed in Phase 1 (neighbor lookup arrives in Phase 2).
	void BuildChunkMesh(const FTPChunk& Chunk, FTPChunkMeshData& Out);
}

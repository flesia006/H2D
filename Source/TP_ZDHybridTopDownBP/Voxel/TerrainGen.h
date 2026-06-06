#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.h"
#include "Perlin.h"
#include "VoxelEdits.h"

// Seeded heightmap terrain with biomes, caves, and ore veins. Built once on the
// game thread, then shared read-only across worker generation tasks
// (immutable -> thread-safe).
class FTPTerrainGen
{
public:
	FTPTerrainGen(int64 Seed, int32 InHeightMin, int32 InHeightMax);

	// World Z of the surface column at world block (Wx, Wy).
	int32 SurfaceHeight(int32 Wx, int32 Wy) const;

	// Per-biome vertex tint for grass/leaves at world column (Wx, Wy).
	// Multiplies the atlas color in the material (white = no change).
	FColor BiomeTintAt(int32 Wx, int32 Wy) const;

	// Fills a chunk's block array (size ChunkVolume) for the given chunk coord:
	// terrain -> caves -> ore veins -> vegetation, all deterministic from the seed.
	// Blocks that fall outside this chunk (e.g. tree leaves) are appended to
	// OutCrossChunkEdits for the world to route to the neighbor chunk.
	void GenerateChunk(const FIntVector& Coord, TArray<BlockId>& OutBlocks,
		TArray<FPendingEdit>& OutCrossChunkEdits) const;

private:
	void PlaceOres(const FIntVector& Coord, TArray<BlockId>& Blocks) const;
	void GenerateVegetation(const FIntVector& Coord, TArray<BlockId>& Blocks,
		TArray<FPendingEdit>& OutCrossChunkEdits) const;

	int64 WorldSeed;
	int32 HeightMin;
	int32 HeightMax;

	FPerlin Continent;
	FPerlin Hill;
	FPerlin Detail;
	FPerlin Temp;
	FPerlin Humidity;
	FPerlin Cave;
};

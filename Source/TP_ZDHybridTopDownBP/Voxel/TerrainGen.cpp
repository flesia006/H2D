#include "TerrainGen.h"
#include "Chunk.h"
#include "VoxelHash.h"

namespace
{
	constexpr uint32 SALT_CONTINENT = 1001;
	constexpr uint32 SALT_HILL      = 1002;
	constexpr uint32 SALT_DETAIL    = 1003;

	// Noise frequencies (per world block). Lower = broader features.
	constexpr float FREQ_CONTINENT = 0.0125f;
	constexpr float FREQ_HILL      = 0.05f;
	constexpr float FREQ_DETAIL    = 0.15f;
}

FTPTerrainGen::FTPTerrainGen(int64 Seed, int32 InHeightMin, int32 InHeightMax)
	: Continent(VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_CONTINENT))
	, Hill     (VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_HILL))
	, Detail   (VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_DETAIL))
	, HeightMin(InHeightMin)
	, HeightMax(FMath::Max(InHeightMax, InHeightMin + 1))
{
}

int32 FTPTerrainGen::SurfaceHeight(int32 Wx, int32 Wy) const
{
	const float Fx = static_cast<float>(Wx);
	const float Fy = static_cast<float>(Wy);

	const float C = Continent.Noise2D(Fx * FREQ_CONTINENT, Fy * FREQ_CONTINENT);
	const float H = Hill.Noise2D     (Fx * FREQ_HILL,      Fy * FREQ_HILL)      * 0.5f;
	const float D = Detail.Noise2D   (Fx * FREQ_DETAIL,    Fy * FREQ_DETAIL)    * 0.15f;

	const float U = FMath::Clamp((C + H + D) * 0.5f + 0.5f, 0.f, 1.f);
	return FMath::RoundToInt(FMath::Lerp(static_cast<float>(HeightMin), static_cast<float>(HeightMax), U));
}

void FTPTerrainGen::GenerateChunk(const FIntVector& Coord, TArray<BlockId>& OutBlocks) const
{
	using namespace VoxelConst;
	OutBlocks.SetNumUninitialized(ChunkVolume);

	const int32 BaseX = Coord.X * ChunkSize;
	const int32 BaseY = Coord.Y * ChunkSize;
	const int32 BaseZ = Coord.Z * ChunkSize;

	for (int32 y = 0; y < ChunkSize; ++y)
	for (int32 x = 0; x < ChunkSize; ++x)
	{
		const int32 Surf = SurfaceHeight(BaseX + x, BaseY + y);

		for (int32 z = 0; z < ChunkSize; ++z)
		{
			const int32 Wz = BaseZ + z;

			ETPBlockId Id;
			if      (Wz < Surf - 4) Id = ETPBlockId::Stone;
			else if (Wz < Surf)     Id = ETPBlockId::Dirt;
			else if (Wz == Surf)    Id = ETPBlockId::Grass;
			else                    Id = ETPBlockId::Air;

			OutBlocks[FTPChunk::Index(x, y, z)] = static_cast<BlockId>(Id);
		}
	}
}

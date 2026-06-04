#include "TerrainGen.h"
#include "Chunk.h"
#include "VoxelHash.h"
#include "VoxelCoord.h"
#include "Math/RandomStream.h"

namespace
{
	constexpr uint32 SALT_CONTINENT = 1001;
	constexpr uint32 SALT_HILL      = 1002;
	constexpr uint32 SALT_DETAIL    = 1003;
	constexpr uint32 SALT_TEMP      = 2001;
	constexpr uint32 SALT_HUMIDITY  = 2002;
	constexpr uint32 SALT_CAVE      = 3001;
	constexpr uint32 SALT_TREE      = 5001;

	// Noise frequencies (per world block). Lower = broader features.
	constexpr float FREQ_CONTINENT = 0.0125f;
	constexpr float FREQ_HILL      = 0.05f;
	constexpr float FREQ_DETAIL    = 0.15f;
	constexpr float FREQ_BIOME     = 0.0040f; // temp/humidity vary slowly
	constexpr float FREQ_CAVE      = 0.06f;

	// 3D cave noise above this value (range ~[-1,1]) is carved to Air.
	constexpr float CAVE_THRESHOLD = 0.35f;

	enum class EBiome : uint8
	{
		Tundra, Taiga, SnowSwamp,
		Plains, Forest, Swamp,
		Desert, Savanna, Jungle
	};

	EBiome PickBiome(float TempV, float HumV)
	{
		const int32 T = TempV < -0.33f ? 0 : (TempV < 0.33f ? 1 : 2);
		const int32 H = HumV  < -0.33f ? 0 : (HumV  < 0.33f ? 1 : 2);
		return static_cast<EBiome>(T * 3 + H);
	}

	// Surface block per biome. (Snow biomes reuse Grass until a Snow block exists.)
	ETPBlockId SurfaceBlockFor(EBiome B)
	{
		switch (B)
		{
			case EBiome::Desert:
			case EBiome::Savanna:
				return ETPBlockId::Sand;
			default:
				return ETPBlockId::Grass;
		}
	}

	struct FOreVein
	{
		ETPBlockId Block;
		uint32 Salt;
		int32 Tries;        // attempts per chunk
		int32 ClusterSize;  // blocks per attempt
		int32 ZMin, ZMax;   // world Z band
	};

	// Coal common & shallow, gold rare & deep.
	const FOreVein Veins[] = {
		{ ETPBlockId::CoalOre, 4001, 6, 6, -64, 24 },
		{ ETPBlockId::IronOre, 4002, 3, 5, -64, 16 },
		{ ETPBlockId::GoldOre, 4003, 1, 4, -64,  4 },
	};

	// Places a block at world coords, into this chunk if it lands inside it
	// (only over Air), otherwise queued as a cross-chunk edit.
	void EmitBlock(int32 Wx, int32 Wy, int32 Wz, ETPBlockId Block, const FIntVector& Coord,
		TArray<BlockId>& Out, TArray<FPendingEdit>& Edits)
	{
		const FIntVector Wb(Wx, Wy, Wz);
		const FIntVector Cc = VoxelCoord::WorldBlockToChunk(Wb);
		if (Cc == Coord)
		{
			const FIntVector L = VoxelCoord::WorldBlockToLocal(Wb);
			BlockId& B = Out[FTPChunk::Index(L.X, L.Y, L.Z)];
			if (B == static_cast<BlockId>(ETPBlockId::Air))
			{
				B = static_cast<BlockId>(Block);
			}
		}
		else
		{
			Edits.Add(FPendingEdit{ Wb, static_cast<uint8>(Block) });
		}
	}
}

FTPTerrainGen::FTPTerrainGen(int64 Seed, int32 InHeightMin, int32 InHeightMax)
	: WorldSeed(Seed)
	, HeightMin(InHeightMin)
	, HeightMax(FMath::Max(InHeightMax, InHeightMin + 1))
	, Continent(VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_CONTINENT))
	, Hill     (VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_HILL))
	, Detail   (VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_DETAIL))
	, Temp     (VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_TEMP))
	, Humidity (VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_HUMIDITY))
	, Cave     (VoxelNoise::DeriveSeed(Seed, 0, 0, 0, SALT_CAVE))
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

void FTPTerrainGen::GenerateChunk(const FIntVector& Coord, TArray<BlockId>& OutBlocks,
	TArray<FPendingEdit>& OutCrossChunkEdits) const
{
	using namespace VoxelConst;
	OutBlocks.SetNumUninitialized(ChunkVolume);

	const int32 BaseX = Coord.X * ChunkSize;
	const int32 BaseY = Coord.Y * ChunkSize;
	const int32 BaseZ = Coord.Z * ChunkSize;

	for (int32 y = 0; y < ChunkSize; ++y)
	for (int32 x = 0; x < ChunkSize; ++x)
	{
		const int32 Wx = BaseX + x;
		const int32 Wy = BaseY + y;
		const int32 Surf = SurfaceHeight(Wx, Wy);

		const float TempV = Temp.Noise2D(Wx * FREQ_BIOME, Wy * FREQ_BIOME);
		const float HumV  = Humidity.Noise2D(Wx * FREQ_BIOME, Wy * FREQ_BIOME);
		const EBiome Biome = PickBiome(TempV, HumV);
		const ETPBlockId SurfBlock = SurfaceBlockFor(Biome);
		const ETPBlockId SubBlock  = (SurfBlock == ETPBlockId::Sand) ? ETPBlockId::Sand : ETPBlockId::Dirt;

		for (int32 z = 0; z < ChunkSize; ++z)
		{
			const int32 Wz = BaseZ + z;

			ETPBlockId Id;
			if      (Wz < Surf - 4) Id = ETPBlockId::Stone;
			else if (Wz < Surf)     Id = SubBlock;
			else if (Wz == Surf)    Id = SurfBlock;
			else                    Id = ETPBlockId::Air;

			// Carve caves below a 2-block surface cap.
			if (Id != ETPBlockId::Air && Wz < Surf - 2)
			{
				const float N = Cave.Noise3D(Wx * FREQ_CAVE, Wy * FREQ_CAVE, Wz * FREQ_CAVE);
				if (N > CAVE_THRESHOLD)
				{
					Id = ETPBlockId::Air;
				}
			}

			OutBlocks[FTPChunk::Index(x, y, z)] = static_cast<BlockId>(Id);
		}
	}

	PlaceOres(Coord, OutBlocks);
	GenerateVegetation(Coord, OutBlocks, OutCrossChunkEdits);
}

void FTPTerrainGen::GenerateVegetation(const FIntVector& Coord, TArray<BlockId>& Blocks,
	TArray<FPendingEdit>& OutCrossChunkEdits) const
{
	using namespace VoxelConst;
	const int32 BaseX = Coord.X * ChunkSize;
	const int32 BaseY = Coord.Y * ChunkSize;

	// Tree density from the chunk-center biome.
	const float CT = Temp.Noise2D((BaseX + 8) * FREQ_BIOME, (BaseY + 8) * FREQ_BIOME);
	const float CH = Humidity.Noise2D((BaseX + 8) * FREQ_BIOME, (BaseY + 8) * FREQ_BIOME);
	int32 Attempts;
	switch (PickBiome(CT, CH))
	{
		case EBiome::Jungle: Attempts = 8; break;
		case EBiome::Forest: Attempts = 6; break;
		case EBiome::Desert:
		case EBiome::Tundra: Attempts = 0; break;
		default:             Attempts = 2; break;
	}
	if (Attempts == 0)
	{
		return;
	}

	FRandomStream R(static_cast<int32>(
		VoxelNoise::DeriveSeed(WorldSeed, Coord.X, Coord.Y, Coord.Z, SALT_TREE)));

	for (int32 t = 0; t < Attempts; ++t)
	{
		const int32 Lx = R.RandRange(0, ChunkSize - 1);
		const int32 Ly = R.RandRange(0, ChunkSize - 1);
		const int32 Wx = BaseX + Lx;
		const int32 Wy = BaseY + Ly;

		const float TV = Temp.Noise2D(Wx * FREQ_BIOME, Wy * FREQ_BIOME);
		const float HV = Humidity.Noise2D(Wx * FREQ_BIOME, Wy * FREQ_BIOME);
		const EBiome B = PickBiome(TV, HV);
		if (B == EBiome::Desert || B == EBiome::Tundra)
		{
			continue;
		}
		if (SurfaceBlockFor(B) != ETPBlockId::Grass)
		{
			continue; // no trees on sand/water surfaces
		}

		const int32 Surf = SurfaceHeight(Wx, Wy);

		// This chunk owns the tree only if the surface voxel is inside it.
		if (Surf < Coord.Z * ChunkSize || Surf >= Coord.Z * ChunkSize + ChunkSize)
		{
			continue;
		}

		const int32 H = R.RandRange(4, 5); // trunk height

		for (int32 k = 1; k <= H; ++k)
		{
			EmitBlock(Wx, Wy, Surf + k, ETPBlockId::Wood, Coord, Blocks, OutCrossChunkEdits);
		}

		for (int32 dz = H - 1; dz <= H + 1; ++dz)
		{
			const int32 Rad = (dz == H + 1) ? 1 : 2;
			for (int32 dy = -Rad; dy <= Rad; ++dy)
			for (int32 dx = -Rad; dx <= Rad; ++dx)
			{
				EmitBlock(Wx + dx, Wy + dy, Surf + dz, ETPBlockId::Leaves, Coord, Blocks, OutCrossChunkEdits);
			}
		}
	}
}

void FTPTerrainGen::PlaceOres(const FIntVector& Coord, TArray<BlockId>& Blocks) const
{
	using namespace VoxelConst;
	const int32 BaseZ = Coord.Z * ChunkSize;
	const BlockId Stone = static_cast<BlockId>(ETPBlockId::Stone);

	for (const FOreVein& V : Veins)
	{
		const uint64 H = VoxelNoise::DeriveSeed(WorldSeed, Coord.X, Coord.Y, Coord.Z, V.Salt);
		FRandomStream R(static_cast<int32>(H));

		for (int32 t = 0; t < V.Tries; ++t)
		{
			const int32 Cx = R.RandRange(0, ChunkSize - 1);
			const int32 Cy = R.RandRange(0, ChunkSize - 1);
			const int32 Cz = R.RandRange(0, ChunkSize - 1);

			const int32 Wz = BaseZ + Cz;
			if (Wz < V.ZMin || Wz > V.ZMax)
			{
				continue;
			}

			// Short random walk to form a cluster.
			for (int32 k = 0; k < V.ClusterSize; ++k)
			{
				const int32 Ox = FMath::Clamp(Cx + R.RandRange(-1, 1), 0, ChunkSize - 1);
				const int32 Oy = FMath::Clamp(Cy + R.RandRange(-1, 1), 0, ChunkSize - 1);
				const int32 Oz = FMath::Clamp(Cz + R.RandRange(-1, 1), 0, ChunkSize - 1);

				BlockId& B = Blocks[FTPChunk::Index(Ox, Oy, Oz)];
				if (B == Stone)
				{
					B = static_cast<BlockId>(V.Block);
				}
			}
		}
	}
}

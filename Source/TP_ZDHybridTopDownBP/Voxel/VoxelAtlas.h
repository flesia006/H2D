#pragma once

#include "CoreMinimal.h"

// Texture atlas layout shared by the block registry, the mesher, and the
// generated T_VoxelAtlas texture. Square atlas, Cols == Rows tiles.
namespace VoxelAtlas
{
	constexpr int32 Cols = 4;
	constexpr int32 Rows = 4;

	// Tile indices (row-major from the top-left of the atlas).
	enum ETile : int32
	{
		Stone     = 0,
		Dirt      = 1,
		GrassTop  = 2,
		GrassSide = 3,
		Sand      = 4,
		Wood      = 5,
		Leaves    = 6,
		Water     = 7,
		CoalOre   = 8,
		IronOre   = 9,
		GoldOre   = 10,
	};

	// Fills a face's 4 UVs for the given tile, ordered to match the mesher's
	// vertex winding: (u0,v0)(u1,v0)(u1,v1)(u0,v1). A tiny inset avoids bleeding.
	FORCEINLINE void TileUV(int32 Tile, FVector2D Out[4])
	{
		const int32 Col = Tile % Cols;
		const int32 Row = Tile / Cols;

		constexpr float Pad = 0.002f;
		const float U0 = (Col + Pad) / Cols;
		const float U1 = (Col + 1.f - Pad) / Cols;
		const float V0 = (Row + Pad) / Rows;
		const float V1 = (Row + 1.f - Pad) / Rows;

		Out[0] = FVector2D(U0, V0);
		Out[1] = FVector2D(U1, V0);
		Out[2] = FVector2D(U1, V1);
		Out[3] = FVector2D(U0, V1);
	}
}

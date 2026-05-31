#pragma once

#include "CoreMinimal.h"

// 1-byte block id. Up to 256 block kinds.
using BlockId = uint8;

enum class ETPBlockId : uint8
{
	Air = 0,
	Stone,
	Dirt,
	Grass,
	Sand,
	Water,
	Wood,
	Leaves,
	Count
};

namespace VoxelConst
{
	constexpr int32 ChunkSize   = 16;
	constexpr int32 ChunkArea   = ChunkSize * ChunkSize;        // 256
	constexpr int32 ChunkVolume = ChunkArea * ChunkSize;        // 4096
	constexpr float BlockSize   = 100.0f;                       // cm (1 UE unit grid)
}

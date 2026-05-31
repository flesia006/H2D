#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.h"

// 16x16x16 block container. Linear index = X + Y*16 + Z*256.
struct FTPChunk
{
	FIntVector      Coord = FIntVector::ZeroValue;
	TArray<BlockId> Blocks;
	bool            bDirty    = false; // needs remesh
	bool            bModified = false; // user-edited (save target)

	FTPChunk()
	{
		Blocks.Init(static_cast<BlockId>(ETPBlockId::Air), VoxelConst::ChunkVolume);
	}

	static FORCEINLINE int32 Index(int32 X, int32 Y, int32 Z)
	{
		return X + Y * VoxelConst::ChunkSize + Z * VoxelConst::ChunkArea;
	}

	FORCEINLINE BlockId Get(int32 X, int32 Y, int32 Z) const
	{
		return Blocks[Index(X, Y, Z)];
	}

	FORCEINLINE void Set(int32 X, int32 Y, int32 Z, BlockId Id)
	{
		Blocks[Index(X, Y, Z)] = Id;
		bDirty = true;
		bModified = true;
	}
};

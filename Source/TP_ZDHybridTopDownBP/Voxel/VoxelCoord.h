#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.h"

// World/chunk/local coordinate conversions.
//   world block : integer block grid (whole world)
//   chunk       : chunk grid coords
//   local       : block coords inside a chunk, always [0, ChunkSize)
// Floor-division / positive-modulo keep the identity
//   chunk * ChunkSize + local == world block   for negative coords too.
namespace VoxelCoord
{
	FORCEINLINE int32 FloorDiv(int32 V, int32 S)
	{
		return (V >= 0) ? (V / S) : ((V - S + 1) / S);
	}

	FORCEINLINE int32 PosMod(int32 V, int32 S)
	{
		const int32 R = V % S;
		return (R < 0) ? R + S : R;
	}

	FORCEINLINE FIntVector WorldBlockToChunk(const FIntVector& Wb)
	{
		const int32 S = VoxelConst::ChunkSize;
		return FIntVector(FloorDiv(Wb.X, S), FloorDiv(Wb.Y, S), FloorDiv(Wb.Z, S));
	}

	FORCEINLINE FIntVector WorldBlockToLocal(const FIntVector& Wb)
	{
		const int32 S = VoxelConst::ChunkSize;
		return FIntVector(PosMod(Wb.X, S), PosMod(Wb.Y, S), PosMod(Wb.Z, S));
	}

	FORCEINLINE FVector ChunkOriginToWorld(const FIntVector& Cc)
	{
		const float Span = VoxelConst::ChunkSize * VoxelConst::BlockSize;
		return FVector(Cc.X * Span, Cc.Y * Span, Cc.Z * Span);
	}

	FORCEINLINE FIntVector WorldPosToBlock(const FVector& P)
	{
		const float B = VoxelConst::BlockSize;
		return FIntVector(
			FMath::FloorToInt(P.X / B),
			FMath::FloorToInt(P.Y / B),
			FMath::FloorToInt(P.Z / B));
	}
}

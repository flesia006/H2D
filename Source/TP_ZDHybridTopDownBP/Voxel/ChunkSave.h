#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.h"

// One user-edited voxel: position within a chunk + the new block id.
struct FBlockEdit
{
	uint16  LocalIndex;
	BlockId NewId;
};

// Per-chunk save delta: only blocks the player changed vs procedural generation.
struct FChunkSave
{
	FIntVector         Coord = FIntVector::ZeroValue;
	TArray<FBlockEdit> Edits;

	friend FArchive& operator<<(FArchive& Ar, FChunkSave& S)
	{
		Ar << S.Coord;
		int32 N = S.Edits.Num();
		Ar << N;
		if (Ar.IsLoading())
		{
			S.Edits.SetNum(N);
		}
		for (int32 i = 0; i < N; ++i)
		{
			Ar << S.Edits[i].LocalIndex;
			Ar << S.Edits[i].NewId;
		}
		return Ar;
	}
};

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "VoxelTypes.h"
#include "Chunk.h"
#include "ChunkMeshData.h"
#include "VoxelMesh.h"
#include "TerrainGen.h"

// Generates a chunk's block data on a worker thread using the shared, immutable
// seeded terrain generator. Touches no mutable shared state.
class FChunkGenTask : public FNonAbandonableTask
{
public:
	friend class FAsyncTask<FChunkGenTask>;

	FChunkGenTask(FIntVector InCoord, TSharedPtr<const FTPTerrainGen, ESPMode::ThreadSafe> InGen)
		: Coord(InCoord), Gen(MoveTemp(InGen))
	{
	}

	// Outputs: filled by DoWork, consumed on the game thread.
	TArray<BlockId> Blocks;
	TArray<FPendingEdit> CrossEdits; // blocks that land in neighbor chunks

	void DoWork()
	{
		Gen->GenerateChunk(Coord, Blocks, CrossEdits);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FChunkGenTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	FIntVector Coord;
	TSharedPtr<const FTPTerrainGen, ESPMode::ThreadSafe> Gen;
};

// Meshes a chunk from a self-contained padded snapshot on a worker thread.
class FChunkMeshTask : public FNonAbandonableTask
{
public:
	friend class FAsyncTask<FChunkMeshTask>;

	FChunkMeshTask(TArray<BlockId>&& InPadded, TArray<FColor>&& InColumnTint)
		: Padded(MoveTemp(InPadded)), ColumnTint(MoveTemp(InColumnTint))
	{
	}

	// Output: built by DoWork, applied on the game thread.
	FTPChunkMeshData Mesh;

	void DoWork()
	{
		VoxelMesh::BuildChunkMeshPadded(Padded, ColumnTint, Mesh);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FChunkMeshTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	TArray<BlockId> Padded;
	TArray<FColor>  ColumnTint;
};

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "VoxelTypes.h"
#include "Chunk.h"
#include "ChunkMeshData.h"
#include "VoxelMesh.h"

// Generates a chunk's block data on a worker thread (flat world for now;
// Phase 3 replaces the fill with seeded noise). Touches no shared state.
class FChunkGenTask : public FNonAbandonableTask
{
public:
	friend class FAsyncTask<FChunkGenTask>;

	FChunkGenTask(FIntVector InCoord, int32 InSurfaceZ)
		: Coord(InCoord), SurfaceZ(InSurfaceZ)
	{
	}

	// Output: filled by DoWork, moved out on the game thread.
	TArray<BlockId> Blocks;

	void DoWork()
	{
		using namespace VoxelConst;
		Blocks.Init(static_cast<BlockId>(ETPBlockId::Air), ChunkVolume);

		for (int32 z = 0; z < ChunkSize; ++z)
		{
			const int32 Wz = Coord.Z * ChunkSize + z;

			ETPBlockId Block;
			if (Wz > SurfaceZ)          Block = ETPBlockId::Air;
			else if (Wz == SurfaceZ)    Block = ETPBlockId::Grass;
			else if (Wz > SurfaceZ - 4) Block = ETPBlockId::Dirt;
			else                        Block = ETPBlockId::Stone;

			if (Block == ETPBlockId::Air)
			{
				continue;
			}

			const BlockId Id = static_cast<BlockId>(Block);
			for (int32 y = 0; y < ChunkSize; ++y)
			for (int32 x = 0; x < ChunkSize; ++x)
			{
				Blocks[FTPChunk::Index(x, y, z)] = Id;
			}
		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FChunkGenTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	FIntVector Coord;
	int32 SurfaceZ;
};

// Meshes a chunk from a self-contained padded snapshot on a worker thread.
class FChunkMeshTask : public FNonAbandonableTask
{
public:
	friend class FAsyncTask<FChunkMeshTask>;

	explicit FChunkMeshTask(TArray<BlockId>&& InPadded)
		: Padded(MoveTemp(InPadded))
	{
	}

	// Output: built by DoWork, applied on the game thread.
	FTPChunkMeshData Mesh;

	void DoWork()
	{
		VoxelMesh::BuildChunkMeshPadded(Padded, Mesh);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FChunkMeshTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	TArray<BlockId> Padded;
};

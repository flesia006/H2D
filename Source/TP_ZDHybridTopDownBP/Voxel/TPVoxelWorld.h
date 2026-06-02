#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Async/AsyncWork.h"
#include "Chunk.h"
#include "TPVoxelWorld.generated.h"

class ATPChunkActor;
class UMaterialInterface;
class FChunkGenTask;
class FChunkMeshTask;
class FTPTerrainGen;

// Streams voxel chunks around a tracked center (player pawn or this actor).
// Generation and meshing run on worker threads; the game thread only builds
// neighbor snapshots, kicks tasks, and applies finished meshes.
UCLASS()
class TP_ZDHYBRIDTOPDOWNBP_API ATPVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	ATPVoxelWorld();

	UPROPERTY(EditAnywhere, Category = "Voxel")
	int64 Seed = 1337;

	// Horizontal load radius in chunks.
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "1"))
	int32 LoadRadiusChunks = 3;

	// Spawned chunk actors use this class (defaults to ATPChunkActor).
	UPROPERTY(EditAnywhere, Category = "Voxel")
	TSubclassOf<ATPChunkActor> ChunkActorClass;

	// Passed to each spawned chunk actor.
	UPROPERTY(EditAnywhere, Category = "Voxel")
	UMaterialInterface* MasterMaterial = nullptr;

	// Generation tasks kicked per frame.
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "1"))
	int32 GenKicksPerTick = 4;

	// Mesh tasks kicked per frame.
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = "1"))
	int32 MeshKicksPerTick = 4;

	// Terrain surface height range, in world blocks. Controls the loaded
	// vertical chunk band, so keep the span modest.
	UPROPERTY(EditAnywhere, Category = "Voxel")
	int32 HeightMin = 6;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	int32 HeightMax = 26;

	// Follow the local player pawn; otherwise center on this actor's location.
	UPROPERTY(EditAnywhere, Category = "Voxel")
	bool bFollowPlayer = true;

	virtual void Tick(float DeltaSeconds) override;

	// Read a block at world block coords; Air for unloaded chunks.
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	uint8 GetBlockWorld(FIntVector WorldBlock) const;

	// Write a block at world block coords; marks the chunk (and border neighbors) dirty.
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetBlockWorld(FIntVector WorldBlock, uint8 BlockId);

	// Line-traces the voxel meshes. OutHitBlock = the block hit (for mining),
	// OutPlaceBlock = the empty cell against the hit face (for placement).
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool RaycastBlock(FVector Start, FVector Direction, float Distance,
		FIntVector& OutHitBlock, FIntVector& OutPlaceBlock);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TMap<FIntVector, TSharedPtr<FTPChunk>> Chunks;
	TMap<FIntVector, TWeakObjectPtr<ATPChunkActor>> Actors;

	TMap<FIntVector, FAsyncTask<FChunkGenTask>*>  GenTasks;
	TMap<FIntVector, FAsyncTask<FChunkMeshTask>*> MeshTasks;

	TArray<FIntVector> LoadQueue;          // nearest-first
	FIntVector LastCenterChunk = FIntVector(MAX_int32);

	TSharedPtr<FTPTerrainGen, ESPMode::ThreadSafe> Terrain;

	FIntVector GetCenterChunk() const;
	void VerticalChunkRange(int32& OutMin, int32& OutMax) const;
	void RefreshLoadQueue(const FIntVector& Center);
	void UnloadFarFrom(const FIntVector& Center);

	void KickGenTasks();
	void CollectGenTasks();
	void KickMeshTasks();
	void CollectMeshTasks();

	TArray<BlockId> BuildPaddedSnapshot(const FIntVector& Coord) const;
	ATPChunkActor* EnsureActor(const FIntVector& Coord);
	void MarkNeighborsDirty(const FIntVector& Coord);
	void CancelTasksAt(const FIntVector& Coord);
	void DrainAllTasks();
};

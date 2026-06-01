#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Chunk.h"
#include "TPChunkActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
struct FTPChunkMeshData;

UCLASS()
class TP_ZDHYBRIDTOPDOWNBP_API ATPChunkActor : public AActor
{
	GENERATED_BODY()

public:
	ATPChunkActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	UProceduralMeshComponent* Mesh;

	// Material with VertexColor -> BaseColor (and Two Sided ON for Phase 1).
	UPROPERTY(EditAnywhere, Category = "Voxel")
	UMaterialInterface* MasterMaterial = nullptr;

	// Phase 1 helper: fill a test chunk and build the mesh when placed/edited in-editor.
	UPROPERTY(EditAnywhere, Category = "Voxel")
	bool bGenerateTestDataOnConstruct = true;

	// In-memory chunk this actor renders.
	FTPChunk Chunk;

	// Rebuilds the mesh from the current Chunk and applies it.
	void RebuildAndApply();

	// Applies a prebuilt mesh (used by ATPVoxelWorld, which meshes with neighbor data).
	void ApplyMesh(const FTPChunkMeshData& Data);

	// Fills Chunk with a simple test pattern (dirt below, grass on top).
	void FillTestData();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};

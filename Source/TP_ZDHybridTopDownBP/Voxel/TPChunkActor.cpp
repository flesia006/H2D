#include "TPChunkActor.h"
#include "VoxelMesh.h"
#include "ChunkMeshData.h"
#include "ProceduralMeshComponent.h"

ATPChunkActor::ATPChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->bUseAsyncCooking = true;
	SetRootComponent(Mesh);
}

void ATPChunkActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bGenerateTestDataOnConstruct)
	{
		FillTestData();
		RebuildAndApply();
	}
}

void ATPChunkActor::FillTestData()
{
	using namespace VoxelConst;

	Chunk = FTPChunk();

	// Dirt for the bottom 8 layers...
	for (int32 z = 0; z < 8; ++z)
	for (int32 y = 0; y < ChunkSize; ++y)
	for (int32 x = 0; x < ChunkSize; ++x)
	{
		Chunk.Set(x, y, z, static_cast<BlockId>(ETPBlockId::Dirt));
	}

	// ...grass cap on layer 8.
	for (int32 y = 0; y < ChunkSize; ++y)
	for (int32 x = 0; x < ChunkSize; ++x)
	{
		Chunk.Set(x, y, 8, static_cast<BlockId>(ETPBlockId::Grass));
	}
}

void ATPChunkActor::RebuildAndApply()
{
	FTPChunkMeshData Data;
	VoxelMesh::BuildChunkMesh(Chunk, Data);

	Mesh->ClearAllMeshSections();
	Mesh->CreateMeshSection_LinearColor(
		/*SectionIndex*/ 0,
		Data.Vertices,
		Data.Triangles,
		Data.Normals,
		Data.UV0,
		Data.Colors,
		/*Tangents*/ TArray<FProcMeshTangent>(),
		/*bCreateCollision*/ true);

	if (MasterMaterial)
	{
		Mesh->SetMaterial(0, MasterMaterial);
	}
}

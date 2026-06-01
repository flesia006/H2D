#include "TPVoxelWorld.h"
#include "TPChunkActor.h"
#include "VoxelCoord.h"
#include "VoxelMesh.h"
#include "ChunkMeshData.h"
#include "ChunkGenTask.h"
#include "BlockType.h"
#include "Kismet/GameplayStatics.h"

ATPVoxelWorld::ATPVoxelWorld()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATPVoxelWorld::BeginPlay()
{
	Super::BeginPlay();

	// Prime the block registry on the game thread so worker meshing never
	// races its lazy initialization.
	FTPBlockRegistry::Get(ETPBlockId::Air);

	LastCenterChunk = FIntVector(MAX_int32); // force initial fill
}

void ATPVoxelWorld::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DrainAllTasks();
	Super::EndPlay(EndPlayReason);
}

FIntVector ATPVoxelWorld::GetCenterChunk() const
{
	FVector P = GetActorLocation();
	if (bFollowPlayer)
	{
		if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			P = Pawn->GetActorLocation();
		}
	}
	return VoxelCoord::WorldBlockToChunk(VoxelCoord::WorldPosToBlock(P));
}

void ATPVoxelWorld::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FIntVector Center = GetCenterChunk();
	if (Center != LastCenterChunk)
	{
		LastCenterChunk = Center;
		RefreshLoadQueue(Center);
		UnloadFarFrom(Center);
	}

	KickGenTasks();
	CollectGenTasks();
	KickMeshTasks();
	CollectMeshTasks();
}

void ATPVoxelWorld::RefreshLoadQueue(const FIntVector& Center)
{
	LoadQueue.Reset();

	const int32 R = LoadRadiusChunks;
	const int32 RV = LoadRadiusVertical;

	TArray<FIntVector> Wanted;
	Wanted.Reserve((2 * R + 1) * (2 * R + 1) * (2 * RV + 1));
	for (int32 dz = -RV; dz <= RV; ++dz)
	for (int32 dy = -R;  dy <= R;  ++dy)
	for (int32 dx = -R;  dx <= R;  ++dx)
	{
		Wanted.Add(Center + FIntVector(dx, dy, dz));
	}

	Wanted.Sort([&Center](const FIntVector& A, const FIntVector& B)
	{
		const FIntVector DA = A - Center;
		const FIntVector DB = B - Center;
		return (DA.X * DA.X + DA.Y * DA.Y + DA.Z * DA.Z)
		     < (DB.X * DB.X + DB.Y * DB.Y + DB.Z * DB.Z);
	});

	for (const FIntVector& C : Wanted)
	{
		if (!Chunks.Contains(C))
		{
			LoadQueue.Add(C);
		}
	}
}

void ATPVoxelWorld::UnloadFarFrom(const FIntVector& Center)
{
	const int32 R = LoadRadiusChunks + 1;          // +1 hysteresis
	const int32 RV = LoadRadiusVertical + 1;

	TArray<FIntVector> ToRemove;
	for (const auto& KV : Chunks)
	{
		const FIntVector D = KV.Key - Center;
		if (FMath::Abs(D.X) > R || FMath::Abs(D.Y) > R || FMath::Abs(D.Z) > RV)
		{
			ToRemove.Add(KV.Key);
		}
	}

	for (const FIntVector& C : ToRemove)
	{
		CancelTasksAt(C);
		if (TWeakObjectPtr<ATPChunkActor>* Found = Actors.Find(C))
		{
			if (ATPChunkActor* A = Found->Get())
			{
				A->Destroy();
			}
			Actors.Remove(C);
		}
		Chunks.Remove(C);
	}
}

void ATPVoxelWorld::KickGenTasks()
{
	for (int32 i = 0; i < GenKicksPerTick && LoadQueue.Num() > 0; ++i)
	{
		const FIntVector C = LoadQueue[0];
		LoadQueue.RemoveAt(0);

		if (Chunks.Contains(C) || GenTasks.Contains(C))
		{
			continue;
		}

		FAsyncTask<FChunkGenTask>* Task = new FAsyncTask<FChunkGenTask>(C, SurfaceZ);
		Task->StartBackgroundTask();
		GenTasks.Add(C, Task);
	}
}

void ATPVoxelWorld::CollectGenTasks()
{
	TArray<FIntVector> Done;
	for (const auto& KV : GenTasks)
	{
		if (KV.Value->IsDone())
		{
			Done.Add(KV.Key);
		}
	}

	for (const FIntVector& C : Done)
	{
		FAsyncTask<FChunkGenTask>* Task = GenTasks.FindRef(C);
		GenTasks.Remove(C);

		TSharedPtr<FTPChunk> Sp = MakeShared<FTPChunk>();
		Sp->Coord = C;
		Sp->Blocks = MoveTemp(Task->GetTask().Blocks);
		Sp->bDirty = true;
		Chunks.Add(C, Sp);

		// Existing neighbors may now expose/hide border faces.
		MarkNeighborsDirty(C);

		delete Task;
	}
}

void ATPVoxelWorld::KickMeshTasks()
{
	int32 Budget = MeshKicksPerTick;
	for (auto& KV : Chunks)
	{
		if (Budget == 0)
		{
			break;
		}
		if (!KV.Value->bDirty || MeshTasks.Contains(KV.Key))
		{
			continue;
		}

		KV.Value->bDirty = false;

		TArray<BlockId> Padded = BuildPaddedSnapshot(KV.Key);
		FAsyncTask<FChunkMeshTask>* Task = new FAsyncTask<FChunkMeshTask>(MoveTemp(Padded));
		Task->StartBackgroundTask();
		MeshTasks.Add(KV.Key, Task);
		--Budget;
	}
}

void ATPVoxelWorld::CollectMeshTasks()
{
	TArray<FIntVector> Done;
	for (const auto& KV : MeshTasks)
	{
		if (KV.Value->IsDone())
		{
			Done.Add(KV.Key);
		}
	}

	for (const FIntVector& C : Done)
	{
		FAsyncTask<FChunkMeshTask>* Task = MeshTasks.FindRef(C);
		MeshTasks.Remove(C);

		if (Chunks.Contains(C)) // may have unloaded while meshing
		{
			if (ATPChunkActor* Actor = EnsureActor(C))
			{
				Actor->ApplyMesh(Task->GetTask().Mesh);
			}
		}

		delete Task;
	}
}

TArray<BlockId> ATPVoxelWorld::BuildPaddedSnapshot(const FIntVector& Coord) const
{
	using namespace VoxelConst;

	TArray<BlockId> Pad;
	Pad.SetNumUninitialized(VoxelMesh::PaddedVolume);

	for (int32 z = -1; z <= ChunkSize; ++z)
	for (int32 y = -1; y <= ChunkSize; ++y)
	for (int32 x = -1; x <= ChunkSize; ++x)
	{
		const FIntVector Wb(
			Coord.X * ChunkSize + x,
			Coord.Y * ChunkSize + y,
			Coord.Z * ChunkSize + z);
		Pad[VoxelMesh::PaddedIndex(x, y, z)] = GetBlockWorld(Wb);
	}

	return Pad;
}

ATPChunkActor* ATPVoxelWorld::EnsureActor(const FIntVector& Coord)
{
	if (TWeakObjectPtr<ATPChunkActor>* Found = Actors.Find(Coord))
	{
		if (ATPChunkActor* A = Found->Get())
		{
			return A;
		}
	}

	UClass* Cls = ChunkActorClass ? ChunkActorClass.Get() : ATPChunkActor::StaticClass();
	const FTransform Xf(FRotator::ZeroRotator, VoxelCoord::ChunkOriginToWorld(Coord));

	ATPChunkActor* Spawned = GetWorld()->SpawnActorDeferred<ATPChunkActor>(Cls, Xf);
	if (!Spawned)
	{
		return nullptr;
	}
	Spawned->bGenerateTestDataOnConstruct = false; // world supplies the data
	if (MasterMaterial)
	{
		Spawned->MasterMaterial = MasterMaterial;
	}
	Spawned->FinishSpawning(Xf);

	Actors.Add(Coord, Spawned);
	return Spawned;
}

void ATPVoxelWorld::MarkNeighborsDirty(const FIntVector& Coord)
{
	static const FIntVector Dirs[6] = {
		FIntVector( 1, 0, 0), FIntVector(-1, 0, 0),
		FIntVector( 0, 1, 0), FIntVector( 0,-1, 0),
		FIntVector( 0, 0, 1), FIntVector( 0, 0,-1)
	};
	for (const FIntVector& D : Dirs)
	{
		if (TSharedPtr<FTPChunk>* N = Chunks.Find(Coord + D))
		{
			(*N)->bDirty = true;
		}
	}
}

void ATPVoxelWorld::CancelTasksAt(const FIntVector& Coord)
{
	if (FAsyncTask<FChunkGenTask>* Task = GenTasks.FindRef(Coord))
	{
		Task->EnsureCompletion();
		delete Task;
		GenTasks.Remove(Coord);
	}
	if (FAsyncTask<FChunkMeshTask>* Task = MeshTasks.FindRef(Coord))
	{
		Task->EnsureCompletion();
		delete Task;
		MeshTasks.Remove(Coord);
	}
}

void ATPVoxelWorld::DrainAllTasks()
{
	for (const auto& KV : GenTasks)
	{
		KV.Value->EnsureCompletion();
		delete KV.Value;
	}
	GenTasks.Empty();

	for (const auto& KV : MeshTasks)
	{
		KV.Value->EnsureCompletion();
		delete KV.Value;
	}
	MeshTasks.Empty();
}

uint8 ATPVoxelWorld::GetBlockWorld(FIntVector WorldBlock) const
{
	const FIntVector Cc = VoxelCoord::WorldBlockToChunk(WorldBlock);
	if (const TSharedPtr<FTPChunk>* Found = Chunks.Find(Cc))
	{
		const FIntVector L = VoxelCoord::WorldBlockToLocal(WorldBlock);
		return (*Found)->Get(L.X, L.Y, L.Z);
	}
	return static_cast<uint8>(ETPBlockId::Air);
}

void ATPVoxelWorld::SetBlockWorld(FIntVector WorldBlock, uint8 BlockId)
{
	const FIntVector Cc = VoxelCoord::WorldBlockToChunk(WorldBlock);
	TSharedPtr<FTPChunk>* Found = Chunks.Find(Cc);
	if (!Found)
	{
		return;
	}

	const FIntVector L = VoxelCoord::WorldBlockToLocal(WorldBlock);
	(*Found)->Set(L.X, L.Y, L.Z, BlockId); // sets bDirty + bModified

	// If on a chunk border, the touching neighbor's border faces change too.
	const int32 Last = VoxelConst::ChunkSize - 1;
	if (L.X == 0)    { if (TSharedPtr<FTPChunk>* N = Chunks.Find(Cc + FIntVector(-1, 0, 0))) (*N)->bDirty = true; }
	if (L.X == Last) { if (TSharedPtr<FTPChunk>* N = Chunks.Find(Cc + FIntVector( 1, 0, 0))) (*N)->bDirty = true; }
	if (L.Y == 0)    { if (TSharedPtr<FTPChunk>* N = Chunks.Find(Cc + FIntVector( 0,-1, 0))) (*N)->bDirty = true; }
	if (L.Y == Last) { if (TSharedPtr<FTPChunk>* N = Chunks.Find(Cc + FIntVector( 0, 1, 0))) (*N)->bDirty = true; }
	if (L.Z == 0)    { if (TSharedPtr<FTPChunk>* N = Chunks.Find(Cc + FIntVector( 0, 0,-1))) (*N)->bDirty = true; }
	if (L.Z == Last) { if (TSharedPtr<FTPChunk>* N = Chunks.Find(Cc + FIntVector( 0, 0, 1))) (*N)->bDirty = true; }
}

bool ATPVoxelWorld::RaycastBlock(FVector Start, FVector Direction, float Distance,
	FIntVector& OutHitBlock, FIntVector& OutPlaceBlock)
{
	FHitResult Hit;
	const FVector End = Start + Direction.GetSafeNormal() * Distance;

	FCollisionQueryParams Params;
	Params.bTraceComplex = true; // procedural mesh collision is per-triangle
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return false;
	}

	const float Half = VoxelConst::BlockSize * 0.5f;
	OutHitBlock   = VoxelCoord::WorldPosToBlock(Hit.ImpactPoint - Hit.ImpactNormal * Half);
	OutPlaceBlock = VoxelCoord::WorldPosToBlock(Hit.ImpactPoint + Hit.ImpactNormal * Half);
	return true;
}

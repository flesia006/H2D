#include "TPVoxelWorld.h"
#include "TPChunkActor.h"
#include "VoxelCoord.h"
#include "VoxelMesh.h"
#include "ChunkMeshData.h"
#include "ChunkGenTask.h"
#include "TerrainGen.h"
#include "BlockType.h"
#include "ChunkSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"

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

	// Build the immutable terrain generator once; worker tasks share it read-only.
	Terrain = MakeShared<FTPTerrainGen, ESPMode::ThreadSafe>(Seed, HeightMin, HeightMax);

	LastCenterChunk = FIntVector(MAX_int32); // force initial fill
}

void ATPVoxelWorld::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Persist all outstanding user edits before tearing down.
	for (const TPair<FIntVector, TSharedPtr<FTPChunk>>& KV : Chunks)
	{
		SaveChunkDelta(*KV.Value);
	}

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

void ATPVoxelWorld::VerticalChunkRange(int32& OutMin, int32& OutMax) const
{
	// Load a fixed vertical band that fully covers the terrain height range,
	// regardless of where the player is vertically (heightmap world, no caves yet).
	OutMin = VoxelCoord::FloorDiv(0, VoxelConst::ChunkSize) - 1;          // one solid floor chunk
	OutMax = VoxelCoord::FloorDiv(HeightMax, VoxelConst::ChunkSize);
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
	int32 VMin, VMax;
	VerticalChunkRange(VMin, VMax);

	TArray<FIntVector> Wanted;
	Wanted.Reserve((2 * R + 1) * (2 * R + 1) * (VMax - VMin + 1));
	for (int32 cz = VMin; cz <= VMax; ++cz)
	for (int32 dy = -R;   dy <= R;    ++dy)
	for (int32 dx = -R;   dx <= R;    ++dx)
	{
		Wanted.Add(FIntVector(Center.X + dx, Center.Y + dy, cz));
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
	int32 VMin, VMax;
	VerticalChunkRange(VMin, VMax);

	TArray<FIntVector> ToRemove;
	for (const auto& KV : Chunks)
	{
		const FIntVector D = KV.Key - Center;
		if (FMath::Abs(D.X) > R || FMath::Abs(D.Y) > R || KV.Key.Z < VMin || KV.Key.Z > VMax)
		{
			ToRemove.Add(KV.Key);
		}
	}

	for (const FIntVector& C : ToRemove)
	{
		CancelTasksAt(C);
		if (TSharedPtr<FTPChunk>* Chunk = Chunks.Find(C))
		{
			SaveChunkDelta(**Chunk); // persist user edits before discarding
		}
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

	// Drop pending edits aimed at chunks that are now far away and unlikely to load.
	for (auto It = PendingForChunk.CreateIterator(); It; ++It)
	{
		const FIntVector D = It.Key() - Center;
		if (FMath::Abs(D.X) > R || FMath::Abs(D.Y) > R || It.Key().Z < VMin || It.Key().Z > VMax)
		{
			It.RemoveCurrent();
		}
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

		FAsyncTask<FChunkGenTask>* Task = new FAsyncTask<FChunkGenTask>(C, Terrain);
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

		// Apply edits that earlier-generated neighbors queued for this chunk.
		if (TArray<FPendingEdit>* List = PendingForChunk.Find(C))
		{
			for (const FPendingEdit& E : *List)
			{
				ApplyEdit(*Sp, E);
			}
			PendingForChunk.Remove(C);
		}

		// User edits from a previous session win over procedural + vegetation.
		LoadAndApplyDelta(*Sp);

		Chunks.Add(C, Sp);

		// Route this chunk's own cross-chunk edits (tree parts spilling outward).
		for (const FPendingEdit& E : Task->GetTask().CrossEdits)
		{
			RouteCrossEdit(E);
		}

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

		TArray<BlockId> Padded;
		TArray<FColor> ColumnTint;
		BuildSnapshot(KV.Key, Padded, ColumnTint);
		FAsyncTask<FChunkMeshTask>* Task =
			new FAsyncTask<FChunkMeshTask>(MoveTemp(Padded), MoveTemp(ColumnTint));
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

void ATPVoxelWorld::BuildSnapshot(const FIntVector& Coord,
	TArray<BlockId>& OutPadded, TArray<FColor>& OutColumnTint) const
{
	using namespace VoxelConst;

	OutPadded.SetNumUninitialized(VoxelMesh::PaddedVolume);
	for (int32 z = -1; z <= ChunkSize; ++z)
	for (int32 y = -1; y <= ChunkSize; ++y)
	for (int32 x = -1; x <= ChunkSize; ++x)
	{
		const FIntVector Wb(
			Coord.X * ChunkSize + x,
			Coord.Y * ChunkSize + y,
			Coord.Z * ChunkSize + z);
		OutPadded[VoxelMesh::PaddedIndex(x, y, z)] = GetBlockWorld(Wb);
	}

	// Per-column biome tint for this chunk's own 16x16 footprint.
	OutColumnTint.SetNumUninitialized(ChunkArea);
	for (int32 y = 0; y < ChunkSize; ++y)
	for (int32 x = 0; x < ChunkSize; ++x)
	{
		const FColor Tint = Terrain.IsValid()
			? Terrain->BiomeTintAt(Coord.X * ChunkSize + x, Coord.Y * ChunkSize + y)
			: FColor::White;
		OutColumnTint[x + y * ChunkSize] = Tint;
	}
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

void ATPVoxelWorld::RouteCrossEdit(const FPendingEdit& Edit)
{
	const FIntVector Cc = VoxelCoord::WorldBlockToChunk(Edit.WorldBlock);
	if (TSharedPtr<FTPChunk>* Found = Chunks.Find(Cc))
	{
		ApplyEdit(**Found, Edit); // target already loaded -> apply + remesh
	}
	else
	{
		PendingForChunk.FindOrAdd(Cc).Add(Edit); // apply when it generates
	}
}

void ATPVoxelWorld::ApplyEdit(FTPChunk& Chunk, const FPendingEdit& Edit)
{
	const FIntVector L = VoxelCoord::WorldBlockToLocal(Edit.WorldBlock);
	const int32 Idx = FTPChunk::Index(L.X, L.Y, L.Z);
	if (Chunk.Blocks[Idx] == static_cast<BlockId>(ETPBlockId::Air))
	{
		Chunk.Blocks[Idx] = Edit.Block;
		Chunk.bDirty = true; // generated vegetation, not a user edit -> no bModified
	}
}

FString ATPVoxelWorld::ChunkFilePath(const FIntVector& Coord) const
{
	const FString WorldName = FString::Printf(TEXT("World_%lld"), Seed);
	return FPaths::ProjectSavedDir() / TEXT("VoxelWorlds") / WorldName /
		FString::Printf(TEXT("c_%d_%d_%d.bin"), Coord.X, Coord.Y, Coord.Z);
}

void ATPVoxelWorld::SaveChunkDelta(const FTPChunk& Chunk) const
{
	if (Chunk.Edits.Num() == 0)
	{
		return; // nothing user-changed -> no file
	}

	FChunkSave S;
	S.Coord = Chunk.Coord;
	S.Edits.Reserve(Chunk.Edits.Num());
	for (const TPair<uint16, BlockId>& KV : Chunk.Edits)
	{
		S.Edits.Add(FBlockEdit{ KV.Key, KV.Value });
	}

	TArray<uint8> Bytes;
	FMemoryWriter Ar(Bytes);
	Ar << S;

	const FString Path = ChunkFilePath(Chunk.Coord);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), /*Tree*/ true);
	FFileHelper::SaveArrayToFile(Bytes, *Path);
}

bool ATPVoxelWorld::LoadAndApplyDelta(FTPChunk& Chunk) const
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *ChunkFilePath(Chunk.Coord)))
	{
		return false;
	}

	FMemoryReader Ar(Bytes);
	FChunkSave S;
	Ar << S;

	for (const FBlockEdit& E : S.Edits)
	{
		if (E.LocalIndex < Chunk.Blocks.Num())
		{
			Chunk.Blocks[E.LocalIndex] = E.NewId;
			Chunk.Edits.Add(E.LocalIndex, E.NewId); // keep so re-save persists
		}
	}
	Chunk.bModified = true;
	return true;
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

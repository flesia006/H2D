#include "BlockType.h"
#include "VoxelAtlas.h"

TArray<FTPBlockType> FTPBlockRegistry::Types;
bool FTPBlockRegistry::bInitialized = false;

void FTPBlockRegistry::EnsureInit()
{
	if (bInitialized)
	{
		return;
	}
	bInitialized = true;

	Types.SetNum(static_cast<int32>(ETPBlockId::Count));

	auto Define = [](ETPBlockId Id, const TCHAR* InName, bool bInSolid, bool bInTransparent, FColor InColor)
	{
		FTPBlockType T;
		T.Name = InName;
		T.bSolid = bInSolid;
		T.bTransparent = bInTransparent;
		T.Color = InColor;
		FTPBlockRegistry::Types[static_cast<int32>(Id)] = T;
	};

	//      id                name         solid  transparent  color
	Define(ETPBlockId::Air,    TEXT("Air"),    false, true,  FColor::White);
	Define(ETPBlockId::Stone,  TEXT("Stone"),  true,  false, FColor(128, 128, 128));
	Define(ETPBlockId::Dirt,   TEXT("Dirt"),   true,  false, FColor(120, 85, 55));
	Define(ETPBlockId::Grass,  TEXT("Grass"),  true,  false, FColor(95, 160, 75));
	Define(ETPBlockId::Sand,   TEXT("Sand"),   true,  false, FColor(210, 200, 140));
	Define(ETPBlockId::Water,  TEXT("Water"),  false, true,  FColor(60, 110, 200));
	Define(ETPBlockId::Wood,   TEXT("Wood"),   true,  false, FColor(110, 80, 50));
	Define(ETPBlockId::Leaves, TEXT("Leaves"), true,  false, FColor(70, 130, 60));

	// Atlas tiles. SetTiles(side, top, bottom); most blocks use one tile for all faces.
	auto SetTiles = [](ETPBlockId Id, int32 Side, int32 Top, int32 Bottom)
	{
		FTPBlockType& T = FTPBlockRegistry::Types[static_cast<int32>(Id)];
		T.AtlasIndex[0] = T.AtlasIndex[1] = T.AtlasIndex[2] = T.AtlasIndex[3] = Side;
		T.AtlasIndex[4] = Top;
		T.AtlasIndex[5] = Bottom;
	};

	using namespace VoxelAtlas;
	SetTiles(ETPBlockId::Stone,  Stone,     Stone,    Stone);
	SetTiles(ETPBlockId::Dirt,   Dirt,      Dirt,     Dirt);
	SetTiles(ETPBlockId::Grass,  GrassSide, GrassTop, Dirt);   // green top, dirty sides
	SetTiles(ETPBlockId::Sand,   Sand,      Sand,     Sand);
	SetTiles(ETPBlockId::Water,  Water,     Water,    Water);
	SetTiles(ETPBlockId::Wood,   Wood,      Wood,     Wood);
	SetTiles(ETPBlockId::Leaves, Leaves,    Leaves,   Leaves);
}

const FTPBlockType& FTPBlockRegistry::Get(ETPBlockId Id)
{
	EnsureInit();
	const int32 Index = static_cast<int32>(Id);
	return Types[(Index >= 0 && Index < Types.Num()) ? Index : 0];
}

#pragma once

#include "CoreMinimal.h"
#include "VoxelTypes.h"

// Plain block-property record. Phase 1 keeps this code-defined (no DataTable yet);
// a UDataTable-backed version arrives when block variety/textures grow (Phase 3).
struct FTPBlockType
{
	const TCHAR* Name = TEXT("Air");
	bool   bSolid       = false; // participates in collision/meshing
	bool   bTransparent = true;  // neighbor of a transparent block still draws its face
	FColor Color        = FColor::White; // vertex color (fallback when no atlas)

	// Atlas tile index per face: 0:+X 1:-X 2:+Y 3:-Y 4:+Z(top) 5:-Z(bottom).
	int32  AtlasIndex[6] = { 0, 0, 0, 0, 0, 0 };
};

// Lazy-initialized, code-defined block table.
class FTPBlockRegistry
{
public:
	static const FTPBlockType& Get(ETPBlockId Id);

private:
	static void EnsureInit();
	static TArray<FTPBlockType> Types;
	static bool bInitialized;
};

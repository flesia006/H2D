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
	FColor Color        = FColor::White; // Phase 1 vertex color (stand-in for textures)
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

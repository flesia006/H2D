#pragma once

#include "CoreMinimal.h"

// A "place this block here later" instruction, used for generation that crosses
// chunk boundaries (e.g. tree leaves spilling into a neighbor chunk).
struct FPendingEdit
{
	FIntVector WorldBlock;
	uint8      Block;
};

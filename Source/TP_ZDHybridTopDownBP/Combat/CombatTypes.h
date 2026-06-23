#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

// Core combat stats. Shared by every combatant template (player/enemy alike);
// instances keep their current HP/MP separately in FTPCombatant.
USTRUCT(BlueprintType)
struct TP_ZDHYBRIDTOPDOWNBP_API FTPCombatStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "1"))
	int32 MaxHP = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "0"))
	int32 MaxMP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "0"))
	int32 ATK = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "0"))
	int32 DEF = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "0"))
	int32 MAG = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "0"))
	int32 RES = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "0"))
	int32 SPD = 5;

	// Crit / flee rolls use this as a percent (0..100).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats", meta = (ClampMin = "0", ClampMax = "100"))
	int32 LUK = 5;
};

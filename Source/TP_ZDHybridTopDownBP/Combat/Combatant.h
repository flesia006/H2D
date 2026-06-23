#pragma once

#include "CoreMinimal.h"
#include "CombatantDef.h"
#include "Combatant.generated.h"

// Runtime mutable state for one combatant in an active battle. The unchanging
// template is referenced via Def; current HP/MP and per-turn flags live here.
USTRUCT(BlueprintType)
struct TP_ZDHYBRIDTOPDOWNBP_API FTPCombatant
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UTPCombatantDef> Def = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 CurHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 CurMP = 0;

	// Set for the duration of one round when the combatant picks Defend.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bDefending = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bDead = false;

	void InitFrom(UTPCombatantDef* InDef)
	{
		Def = InDef;
		if (InDef)
		{
			CurHP = InDef->Stats.MaxHP;
			CurMP = InDef->Stats.MaxMP;
		}
		bDefending = false;
		bDead = false;
	}

	int32 GetSPD() const { return Def ? Def->Stats.SPD : 0; }
	bool  IsPlayer() const { return Def && Def->bIsPlayerControlled; }
};

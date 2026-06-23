#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatTypes.h"
#include "CombatantDef.generated.h"

class UPaperFlipbook;

// Authoring template for one combatant kind (a hero or a monster). Editors
// create one DataAsset per kind (e.g. DA_Slime, DA_Hero) and reference it from
// encounters, parties, etc. Runtime state lives separately in FTPCombatant.
UCLASS(BlueprintType)
class TP_ZDHYBRIDTOPDOWNBP_API UTPCombatantDef : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FTPCombatStats Stats;

	// Battle-screen sprite. Soft pointer keeps cooking flexible.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSoftObjectPtr<UPaperFlipbook> Sprite;

	// Player-controlled combatants take input; others use AI in the manager.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsPlayerControlled = false;
};

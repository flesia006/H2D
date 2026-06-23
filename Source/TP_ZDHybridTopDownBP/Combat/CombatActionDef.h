#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatActionDef.generated.h"

// What the action *does* — the resolver branches on this. Power/MPCost are
// supplied by the asset so designers can tune without code changes.
UENUM(BlueprintType)
enum class ETPActionKind : uint8
{
	Attack,
	Defend,
	Run,
	Skill,
	Item,
};

// Who the action targets. Phase 1 only uses Enemy/Self; the rest are scaffolding
// for the multi-combatant Phase 2.
UENUM(BlueprintType)
enum class ETPTargetKind : uint8
{
	Self,
	Enemy,
	AllEnemies,
	Ally,
	AllAllies,
};

// Authoring asset for one combat action (Attack, Defend, Run, later skills).
UCLASS(BlueprintType)
class TP_ZDHYBRIDTOPDOWNBP_API UTPCombatActionDef : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	ETPActionKind Kind = ETPActionKind::Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	ETPTargetKind Target = ETPTargetKind::Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	int32 MPCost = 0;

	// Multiplier on the actor's ATK in the damage formula (1.0 = normal hit).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float Power = 1.0f;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combatant.h"
#include "CombatActionDef.h"
#include "TPCombatManager.generated.h"

UENUM(BlueprintType)
enum class ETPCombatPhase : uint8
{
	Idle,
	StartRound,
	AwaitInput,
	ResolveAction,
	CheckOutcome,
	Victory,
	Defeat,
	Fled,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatPhaseChanged, ETPCombatPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatLog, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatEnded, ETPCombatPhase, Result);

// The single owner of a battle's progression. Holds combatant runtime state,
// drives the turn loop, resolves actions, and broadcasts phase / log events
// for the UI to react to. Stateless across BeginCombat calls — each call resets.
UCLASS()
class TP_ZDHYBRIDTOPDOWNBP_API ATPCombatManager : public AActor
{
	GENERATED_BODY()

public:
	ATPCombatManager();

	// Start a fresh 1v1 fight. Resets all state.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void BeginCombat(UTPCombatantDef* PlayerDef, UTPCombatantDef* EnemyDef);

	// UI calls this on the player's turn. Returns false if it's not the player's
	// turn to act (so the UI can ignore stray clicks). TargetIndex is reserved
	// for Phase 2 (multi-target); ignored in 1v1.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool SubmitPlayerAction(UTPCombatActionDef* Action, int32 TargetIndex = 0);

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatLog OnCombatLog;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatStateChanged OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatEnded OnCombatEnded;

	UFUNCTION(BlueprintPure, Category = "Combat")
	ETPCombatPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetCurrentTurnIndex() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	const FTPCombatant& GetPlayerCombatant() const { return Player; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	const FTPCombatant& GetEnemyCombatant() const { return Enemy; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	FTPCombatant GetPlayer() const { return Player; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	FTPCombatant GetEnemy() const { return Enemy; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAwaitingPlayerInput() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsCombatOver() const;

protected:
	UPROPERTY()
	FTPCombatant Player;

	UPROPERTY()
	FTPCombatant Enemy;

	// 0 = Player, 1 = Enemy. Sorted by SPD at round start (ties: player first).
	UPROPERTY()
	TArray<int32> TurnOrder;

	int32 TurnCursor = 0;
	ETPCombatPhase Phase = ETPCombatPhase::Idle;

	FTPCombatant&       CombatantAt(int32 Idx);
	const FTPCombatant& CombatantAt(int32 Idx) const;
	int32               OpponentOf(int32 Idx) const { return Idx == 0 ? 1 : 0; }

	void EnterPhase(ETPCombatPhase Next);
	void StartNewRound();
	// Auto-resolves enemy turns until we reach a terminal phase or the player
	// needs to act, at which point we settle on AwaitInput and return.
	void RunUntilInputOrEnd();
	void ResolveAction(ETPActionKind Kind, float Power, int32 ActorIdx, int32 TargetIdx);
	void AdvanceTurn();
	void Log(const FString& Msg);
	void BroadcastStateChanged();

	static int32 ComputeAttackDamage(const FTPCombatant& A, const FTPCombatant& D, float Power);
	static bool  RollFlee(const FTPCombatant& Me, const FTPCombatant& Foe);
	static bool  IsTerminalPhase(ETPCombatPhase InPhase);
};

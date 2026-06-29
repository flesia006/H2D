#include "TPCombatManager.h"

ATPCombatManager::ATPCombatManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATPCombatManager::BeginCombat(UTPCombatantDef* PlayerDef, UTPCombatantDef* EnemyDef)
{
	if (!PlayerDef || !EnemyDef)
	{
		Log(TEXT("BeginCombat: missing combatant defs"));
		return;
	}

	Player.InitFrom(PlayerDef);
	Enemy.InitFrom(EnemyDef);
	BroadcastStateChanged();

	Log(FString::Printf(TEXT("Combat started: %s vs %s"),
		*PlayerDef->DisplayName, *EnemyDef->DisplayName));

	StartNewRound();
	RunUntilInputOrEnd();
}

bool ATPCombatManager::SubmitPlayerAction(UTPCombatActionDef* Action, int32 /*TargetIndex*/)
{
	if (Phase != ETPCombatPhase::AwaitInput) { return false; }
	if (GetCurrentTurnIndex() != 0)          { return false; }
	if (!Action)                              { return false; }

	// Pay MP (Phase 1 actions cost 0 by default).
	if (Action->MPCost > 0)
	{
		if (Player.CurMP < Action->MPCost) { return false; }
		Player.CurMP -= Action->MPCost;
		BroadcastStateChanged();
	}

	ResolveAction(Action->Kind, Action->Power, /*ActorIdx*/ 0, /*TargetIdx*/ 1);

	if (IsTerminalPhase(Phase))
	{
		return true;
	}

	AdvanceTurn();
	RunUntilInputOrEnd();
	return true;
}

int32 ATPCombatManager::GetCurrentTurnIndex() const
{
	if (!TurnOrder.IsValidIndex(TurnCursor)) { return INDEX_NONE; }
	return TurnOrder[TurnCursor];
}

bool ATPCombatManager::IsAwaitingPlayerInput() const
{
	return Phase == ETPCombatPhase::AwaitInput && GetCurrentTurnIndex() == 0;
}

bool ATPCombatManager::IsCombatOver() const
{
	return IsTerminalPhase(Phase);
}

FTPCombatant& ATPCombatManager::CombatantAt(int32 Idx)
{
	return (Idx == 0) ? Player : Enemy;
}

const FTPCombatant& ATPCombatManager::CombatantAt(int32 Idx) const
{
	return (Idx == 0) ? Player : Enemy;
}

void ATPCombatManager::EnterPhase(ETPCombatPhase Next)
{
	Phase = Next;
	OnPhaseChanged.Broadcast(Next);
	if (IsTerminalPhase(Next))
	{
		OnCombatEnded.Broadcast(Next);
	}
}

void ATPCombatManager::StartNewRound()
{
	EnterPhase(ETPCombatPhase::StartRound);

	Player.bDefending = false;
	Enemy.bDefending  = false;
	BroadcastStateChanged();

	TurnOrder = { 0, 1 };
	TurnOrder.Sort([this](const int32& A, const int32& B)
	{
		const int32 SA = CombatantAt(A).GetSPD();
		const int32 SB = CombatantAt(B).GetSPD();
		if (SA != SB) { return SA > SB; }
		return A < B; // ties: player first
	});
	TurnCursor = 0;
}

void ATPCombatManager::RunUntilInputOrEnd()
{
	while (true)
	{
		if (IsTerminalPhase(Phase))
		{
			return;
		}

		// Skip dead combatants (shouldn't happen in 1v1 but keep the loop valid).
		const int32 Idx = GetCurrentTurnIndex();
		if (Idx == INDEX_NONE || CombatantAt(Idx).bDead)
		{
			AdvanceTurn();
			continue;
		}

		if (CombatantAt(Idx).IsPlayer())
		{
			EnterPhase(ETPCombatPhase::AwaitInput);
			return;
		}

		// Enemy AI: always attack the player for Phase 1.
		ResolveAction(ETPActionKind::Attack, /*Power*/ 1.0f, /*Actor*/ Idx, /*Target*/ OpponentOf(Idx));

		if (IsTerminalPhase(Phase))
		{
			return;
		}
		AdvanceTurn();
	}
}

void ATPCombatManager::ResolveAction(ETPActionKind Kind, float Power, int32 ActorIdx, int32 TargetIdx)
{
	EnterPhase(ETPCombatPhase::ResolveAction);
	FTPCombatant& Actor  = CombatantAt(ActorIdx);
	FTPCombatant& Target = CombatantAt(TargetIdx);
	const FString ActorName  = Actor.Def  ? Actor.Def->DisplayName  : TEXT("?");
	const FString TargetName = Target.Def ? Target.Def->DisplayName : TEXT("?");

	switch (Kind)
	{
		case ETPActionKind::Attack:
		{
			const int32 Dmg = ComputeAttackDamage(Actor, Target, Power);
			Target.CurHP = FMath::Max(0, Target.CurHP - Dmg);
			BroadcastStateChanged();
			Log(FString::Printf(TEXT("%s attacks %s for %d damage"),
				*ActorName, *TargetName, Dmg));
			if (Target.CurHP <= 0)
			{
				Target.bDead = true;
				BroadcastStateChanged();
				Log(FString::Printf(TEXT("%s falls"), *TargetName));
				EnterPhase(Target.IsPlayer() ? ETPCombatPhase::Defeat : ETPCombatPhase::Victory);
			}
			break;
		}

		case ETPActionKind::Defend:
		{
			Actor.bDefending = true;
			BroadcastStateChanged();
			Log(FString::Printf(TEXT("%s defends"), *ActorName));
			break;
		}

		case ETPActionKind::Run:
		{
			if (RollFlee(Actor, Target))
			{
				Log(FString::Printf(TEXT("%s fled"), *ActorName));
				EnterPhase(ETPCombatPhase::Fled);
			}
			else
			{
				Log(FString::Printf(TEXT("%s failed to flee"), *ActorName));
			}
			break;
		}

		case ETPActionKind::Skill:
		case ETPActionKind::Item:
			// Phase 2 territory; treat as a wasted turn so the flow keeps moving.
			Log(FString::Printf(TEXT("%s uses an unimplemented action"), *ActorName));
			break;
	}
}

void ATPCombatManager::AdvanceTurn()
{
	EnterPhase(ETPCombatPhase::CheckOutcome);
	++TurnCursor;
	if (TurnCursor >= TurnOrder.Num())
	{
		StartNewRound();
	}
}

void ATPCombatManager::Log(const FString& Msg)
{
	UE_LOG(LogTemp, Log, TEXT("[Combat] %s"), *Msg);
	OnCombatLog.Broadcast(Msg);
}

void ATPCombatManager::BroadcastStateChanged()
{
	OnCombatStateChanged.Broadcast();
}

int32 ATPCombatManager::ComputeAttackDamage(const FTPCombatant& A, const FTPCombatant& D, float Power)
{
	if (!A.Def || !D.Def) { return 1; }
	const int32 Atk = A.Def->Stats.ATK;
	const int32 DefStat = D.bDefending ? (D.Def->Stats.DEF * 3 / 2) : D.Def->Stats.DEF;
	int32 Dmg = FMath::Max(1, FMath::RoundToInt(Atk * Power) - DefStat);
	Dmg = FMath::RoundToInt(Dmg * FMath::FRandRange(0.9f, 1.1f));
	if (FMath::FRand() < A.Def->Stats.LUK / 100.f)
	{
		Dmg = Dmg * 3 / 2; // crit
	}
	return FMath::Max(1, Dmg);
}

bool ATPCombatManager::RollFlee(const FTPCombatant& Me, const FTPCombatant& Foe)
{
	if (!Me.Def) { return false; }
	const float Base = 0.5f + 0.05f * (Me.GetSPD() - Foe.GetSPD()) + Me.Def->Stats.LUK * 0.01f;
	return FMath::FRand() < FMath::Clamp(Base, 0.1f, 0.95f);
}

bool ATPCombatManager::IsTerminalPhase(ETPCombatPhase InPhase)
{
	return InPhase == ETPCombatPhase::Victory ||
		InPhase == ETPCombatPhase::Defeat ||
		InPhase == ETPCombatPhase::Fled;
}

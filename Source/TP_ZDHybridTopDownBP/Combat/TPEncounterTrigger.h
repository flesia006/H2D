#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPEncounterTrigger.generated.h"

class UTPCombatantDef;
class ATPCombatManager;
class USphereComponent;

// One-shot pickup-style trigger: when the player pawn overlaps it, spawn a
// combat manager and start a 1v1 fight, then destroy itself.
UCLASS()
class TP_ZDHYBRIDTOPDOWNBP_API ATPEncounterTrigger : public AActor
{
	GENERATED_BODY()

public:
	ATPEncounterTrigger();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UTPCombatantDef* PlayerDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UTPCombatantDef* EnemyDef = nullptr;

	// Combat manager class to spawn (use a BP subclass that adds UI hooks).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TSubclassOf<ATPCombatManager> CombatManagerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "1"))
	float TriggerRadius = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName PlayerTag = TEXT("Player");

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	USphereComponent* TriggerSphere = nullptr;
};

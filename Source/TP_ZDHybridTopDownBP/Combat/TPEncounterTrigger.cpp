#include "TPEncounterTrigger.h"
#include "TPCombatManager.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"

ATPEncounterTrigger::ATPEncounterTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(TriggerSphere);
}

void ATPEncounterTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!OtherActor || !OtherActor->IsA(APawn::StaticClass()))
	{
		return; // only pawns kick off combat
	}
	if (!PlayerTag.IsNone() && !OtherActor->ActorHasTag(PlayerTag))
	{
		return;
	}
	if (!PlayerDef || !EnemyDef)
	{
		return;
	}

	UClass* MgrClass = CombatManagerClass ? CombatManagerClass.Get() : ATPCombatManager::StaticClass();
	ATPCombatManager* Mgr = GetWorld()->SpawnActor<ATPCombatManager>(MgrClass);
	if (!Mgr)
	{
		return;
	}
	Mgr->BeginCombat(PlayerDef, EnemyDef);

	Destroy(); // one-shot
}

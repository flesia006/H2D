#include "TPCameraRigComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"

UTPCameraRigComponent::UTPCameraRigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTPCameraRigComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		// Auto-wire from the owning Blueprint when references are left unset.
		if (!CameraBoom)
		{
			CameraBoom = Owner->FindComponentByClass<USpringArmComponent>();
		}
		if (!SpriteRoot)
		{
			for (UActorComponent* C : Owner->GetComponents())
			{
				const FString CN = C->GetClass()->GetName();
				if (CN.Contains(TEXT("Flipbook")) || CN.Contains(TEXT("PaperZD")) || CN.Contains(TEXT("Sprite")))
				{
					if (USceneComponent* SC = Cast<USceneComponent>(C))
					{
						SpriteRoot = SC;
						break;
					}
				}
			}
		}
	}

	if (CameraBoom)
	{
		CurrentYaw = TargetYaw = CameraBoom->GetRelativeRotation().Yaw;
	}
}

void UTPCameraRigComponent::RotateCW()
{
	TargetYaw += StepDegrees();
}

void UTPCameraRigComponent::RotateCCW()
{
	TargetYaw -= StepDegrees();
}

void UTPCameraRigComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// InterpSpeed <= 0 makes FInterpTo snap straight to the target.
	CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, InterpSpeed);

	if (CameraBoom)
	{
		FRotator R = CameraBoom->GetRelativeRotation();
		R.Yaw = CurrentYaw; // preserve BP-configured pitch/roll
		CameraBoom->SetRelativeRotation(R);
	}

	UpdateBillboard();
}

void UTPCameraRigComponent::UpdateBillboard()
{
	if (!SpriteRoot)
	{
		return;
	}

	const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!PCM)
	{
		return;
	}

	FVector Dir = PCM->GetCameraLocation() - SpriteRoot->GetComponentLocation();
	Dir.Z = 0.f; // keep the sprite upright
	if (Dir.IsNearlyZero())
	{
		return;
	}

	const FRotator Facing(0.f, Dir.Rotation().Yaw + SpriteYawOffset, 0.f);
	SpriteRoot->SetWorldRotation(Facing);
}

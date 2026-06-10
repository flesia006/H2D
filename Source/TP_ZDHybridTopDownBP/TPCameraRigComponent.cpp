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

FVector UTPCameraRigComponent::GetCameraRelativeMove(FVector2D Input) const
{
	// Use the live camera yaw so "forward" means into the screen, away from camera.
	float Yaw = CurrentYaw;
	if (const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Yaw = PCM->GetCameraRotation().Yaw;
	}

	const FRotationMatrix Rot(FRotator(0.f, Yaw, 0.f));
	const FVector Forward = Rot.GetUnitAxis(EAxis::X);
	const FVector Right   = Rot.GetUnitAxis(EAxis::Y);

	return (Forward * Input.Y + Right * Input.X).GetClampedToMaxSize(1.f);
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

	// Paper2D sprite planes face their local Y axis (not +X), so a 90-degree base
	// correction turns the flat face toward the camera instead of showing it edge-on.
	// If the sprite shows its back, flip SpriteYawOffset by 180.
	const FRotator Facing(0.f, Dir.Rotation().Yaw + 90.f + SpriteYawOffset, 0.f);
	SpriteRoot->SetWorldRotation(Facing);
}

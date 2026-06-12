#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPCameraRigComponent.generated.h"

// Drives an 8-direction (configurable) orbiting camera and billboards a 2D sprite
// to always face the camera. Attach to a Paper/PaperZD character, point CameraBoom
// at the SpringArm and SpriteRoot at the sprite component, then bind RotateCW/CCW
// to the gamepad shoulder buttons (RB/LB).
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class TP_ZDHYBRIDTOPDOWNBP_API UTPCameraRigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTPCameraRigComponent();

	// Scene component whose yaw is driven for the orbit (usually the SpringArm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig")
	USceneComponent* CameraBoom = nullptr;

	// 2D sprite root that billboards toward the camera.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig")
	USceneComponent* SpriteRoot = nullptr;

	// Snap directions around the circle (8 = 45-degree steps).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig", meta = (ClampMin = "2"))
	int32 NumDirections = 8;

	// Easing speed to the target angle. 0 = instant snap.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig", meta = (ClampMin = "0"))
	float InterpSpeed = 10.f;

	// Aligns the sprite's front axis to the camera (try 0 / 90 / 180 / -90).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rig")
	float SpriteYawOffset = 0.f;

	// Step one slot clockwise / counter-clockwise. Bind to RB / LB.
	UFUNCTION(BlueprintCallable, Category = "Camera Rig")
	void RotateCW();

	UFUNCTION(BlueprintCallable, Category = "Camera Rig")
	void RotateCCW();

	// Eased camera yaw — use to make movement camera-relative in Blueprint.
	UFUNCTION(BlueprintPure, Category = "Camera Rig")
	float GetCameraYaw() const { return CurrentYaw; }

	// Converts a screen-relative stick input (X = right, Y = forward/away from
	// camera) into a world-space ground direction, so controls stay intuitive as
	// the camera orbits. Feed the result to AddMovementInput. Magnitude clamped to 1.
	UFUNCTION(BlueprintPure, Category = "Camera Rig")
	FVector GetCameraRelativeMove(FVector2D Input) const;

	// Rotates a world direction (e.g. character velocity) into the camera's frame
	// so the sprite's directional animation matches what's on screen as the camera
	// orbits. Feed this into the PaperZD direction selection. +X = into screen.
	UFUNCTION(BlueprintPure, Category = "Camera Rig")
	FVector GetCameraRelativeDir(FVector WorldDir) const;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	float TargetYaw = 0.f;   // cumulative target (not wrapped)
	float CurrentYaw = 0.f;  // eased current

	float StepDegrees() const { return 360.f / FMath::Max(NumDirections, 2); }
	void UpdateBillboard();
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Core/WoodcraftTypes.h"
#include "UObject/Object.h"
#include "CraftMovement.generated.h"

/**
 * Data-only craft movement module.
 * Numbers and restrictions live here. The crafting component interprets them.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UCraftMovement : public UObject
{
	GENERATED_BODY()

public:

	UCraftMovement();

	/** HeldItems constraint family used by this movement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	ECraftingMotionMode MotionMode = ECraftingMotionMode::Physics;

	ECraftingMotionMode GetMotionMode() const { return MotionMode; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Strength")
	float PlantedLinearStrength = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Strength")
	float PlantedAngularStrength = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Strength")
	float WorkingLinearStrength = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Strength")
	float WorkingAngularStrength = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Locks")
	bool bPlantedLockX = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Locks")
	bool bPlantedLockY = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Locks")
	bool bPlantedLockZ = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Locks")
	bool bWorkingLockX = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Locks")
	bool bWorkingLockY = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Locks")
	bool bWorkingLockZ = false;

	/** Half-extents of the working-hand travel box in craft space. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Volume")
	FVector WorkingVolumeHalfExtents = FVector(12.f, 12.f, 8.f);
};

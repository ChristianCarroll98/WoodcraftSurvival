// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Crafting/Movements/CraftMovement.h"
#include "GrindActiveCraftMovement.generated.h"

/**
 * Two-hand grind. Planted workpiece, working abrader.
 * Always in contact. Progress from XY strokes while the session is live.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UGrindActiveCraftMovement : public UCraftMovement
{
	GENERATED_BODY()

public:

	UGrindActiveCraftMovement();

	/** Travel distance that counts as one stroke before a reverse. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind", meta = (ClampMin = "0.1"))
	float StrokeDistance = 8.f;

	/** Half-extents of the working-hand travel box in craft space. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind")
	FVector WorkingVolumeHalfExtents = FVector(12.f, 12.f, 8.f);

	/** Below this working-hand speed, no progress. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind", meta = (ClampMin = "0.0"))
	float MinStrokeSpeed = 20.f;

	/** Detected stroke speed is clamped to this when applying progress. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind", meta = (ClampMin = "0.0"))
	float MaxStrokeSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind|Strength")
	float PlantedLinearStrength = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind|Strength")
	float PlantedAngularStrength = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind|Strength")
	float WorkingLinearStrength = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind|Strength")
	float WorkingAngularStrength = 400.f;
};

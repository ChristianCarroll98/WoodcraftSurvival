// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Crafting/Movements/CraftMovement.h"
#include "CircleAlternateCraftMovement.generated.h"

/**
 * Opposite-direction circle slices (cordage twist).
 * MotionMode defaults to Guided.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UCircleAlternateCraftMovement : public UCraftMovement
{
	GENERATED_BODY()

public:

	UCircleAlternateCraftMovement();

	/** Radians that complete one slice before the direction flips. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Twist", meta = (ClampMin = "0.1"))
	float SliceRadians = 3.141593f;

	/** Authored circle radius for the working hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Twist", meta = (ClampMin = "0.1"))
	float GestureRadius = 12.f;
};

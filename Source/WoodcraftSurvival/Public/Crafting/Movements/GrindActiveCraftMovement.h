// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Crafting/Movements/CraftMovement.h"
#include "GrindActiveCraftMovement.generated.h"

/**
 * Two-hand grind. Planted workpiece, working abrader.
 * MotionMode defaults to Physics.
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

	/** Minimum working-hand speed that still counts as a stroke. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind", meta = (ClampMin = "0.0"))
	float MinStrokeSpeed = 20.f;

	/** Maximum working-hand speed that still counts as a stroke. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind", meta = (ClampMin = "0.0"))
	float MaxStrokeSpeed = 200.f;

	/** World Z gap closed when Engage is held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grind", meta = (ClampMin = "0.0"))
	float EngageZGap = 4.f;
};

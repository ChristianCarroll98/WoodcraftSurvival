// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "CraftMovement.generated.h"

/**
 * Data-only craft movement module.
 * The crafting component picks a code path with FindMove<T>(Phase).
 * Gesture numbers live on the concrete subclass.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UCraftMovement : public UObject
{
	GENERATED_BODY()

public:

	UCraftMovement();

	/**
	 * After IntroDone, seek the FPArms TwoHanded montage from stage Progress.
	 * Presentations always sample Progress after intro.
	 * Twist leaves this false and writes arms position from slices.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bProgressDrivesMontage = false;
};

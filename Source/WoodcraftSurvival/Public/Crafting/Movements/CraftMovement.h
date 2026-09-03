// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "CraftMovement.generated.h"

/**
 * Data-only craft movement module.
 * The crafting component picks a code path with FindMove<T>().
 * Gesture numbers live on the concrete subclass.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UCraftMovement : public UObject
{
	GENERATED_BODY()

public:

	UCraftMovement();
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemFragment.generated.h"

/**
 * Base class for all item fragments.
 * Fragments are pure data. They describe a capability an item can have.
 * Never put gameplay logic here — that belongs in the components that read the fragments.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UItemFragment : public UObject
{
	GENERATED_BODY()

	// No properties or functions yet.
	// Concrete fragments will add their own data (MaxStackSize, BurnTime, etc.).
};

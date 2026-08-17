// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "StackableFragment.generated.h"

/**
 * Defines how the player carries multiple of this item.
 * ItemInstances themselves never stack — each instance is always one item.
 * This fragment only describes carry limits and visual representation when the player holds several.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UStackableFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	/** Maximum number of this item the player may carry / hold at once. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carrying", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/**
	 * Relative transforms used when visually showing multiple items
	 * (e.g. extra sticks attached while held under the arm).
	 * Index 0 = first extra item, Index 1 = second extra item, etc.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carrying|Visual")
	TArray<FTransform> ISMCMeshTransforms;
};

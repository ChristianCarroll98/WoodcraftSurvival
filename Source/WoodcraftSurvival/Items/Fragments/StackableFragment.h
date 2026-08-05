#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "StackableFragment.generated.h"

/**
 * Allows an item to stack.
 * If this fragment is missing, the item is treated as non-stackable.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UStackableFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	/** Maximum number of items that can exist in one stack. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stacking", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;
};

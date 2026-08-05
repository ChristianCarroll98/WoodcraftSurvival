#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "FuelFragment.generated.h"

/**
 * Marks an item as usable fuel.
 * Systems that interact with fires / campfires / etc. will look for this fragment
 * and read the burn time.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UFuelFragment : public UItemFragment
{
	GENERATED_BODY()
    
    public:
    
	/** How many seconds of burn time this item provides when consumed as fuel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fuel", meta = (ClampMin = "0.0"))
	float BurnTimeSeconds = 120.f;
};

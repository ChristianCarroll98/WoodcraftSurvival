// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "DurabilityFragment.generated.h"

/**
 * Marks an item as having durability (tools, weapons, etc.).
 * Current durability lives on UItemInstance.
 * How much durability is lost on each use is decided by the systems that perform the action.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UDurabilityFragment : public UItemFragment
{
	GENERATED_BODY()

public:
	
	void OnItemSpawned(AItemActor* ItemActor);

	/** Base maximum durability for this item type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Durability", meta = (ClampMin = "1.0"))
	float MaxDurability = 100.f;
};

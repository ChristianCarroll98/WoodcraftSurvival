// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Items/ItemDefinition.h"
#include <UObject/Object.h>
#include "ItemInstance.generated.h"

/**
 * Runtime instance of an item.
 * This is the object that inventories, hands, world actors, and save games actually store.
 * All mutable state (stack count, durability, unique ID, etc.) lives here.
 */
UCLASS(BlueprintType)
class WOODCRAFTSURVIVAL_API UItemInstance : public UObject
{
	GENERATED_BODY()

public:

	/** The static definition this instance was created from. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	TObjectPtr<const UItemDefinition> ItemDefinition;

	/** Globally unique ID for this specific instance (important for saving & networking). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	FGuid UniqueId;

	/** Current durability or health of this instance (only meaningful if the definition has a DurabilityFragment or HealthFragment?). */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Item")
	float CurrentHealth = 100.f;

	/** Forwards the request to the Definition. Returns nullptr if the definition is missing or has no such fragment. */
	template<typename T>
	const T* FindFragment() const
	{
		return ItemDefinition ? ItemDefinition->FindFragment<T>() : nullptr;
	}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

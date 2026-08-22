// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "ItemFragment.h"
#include "DamageItemFragment.generated.h"

/**
 * Provides basic damage information for items that can deal damage
 * (weapons, tools, unarmed, etc.).
 * Actual damage application is handled by the systems that perform the hit.
 * OnItemSpawned enables collision hit events so only items with this fragment
 * generate OnComponentHit callbacks.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UDamageItemFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	/** Type of damage this item deals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UDamageType> DamageType;

	/** Base damage value before any modifiers. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;

	/** Enables hit events + binds the actor's OnItemMeshHit handler on Primary and Secondary meshes. */
	virtual void OnItemSpawned(AItemActor* ItemActor) override;
};

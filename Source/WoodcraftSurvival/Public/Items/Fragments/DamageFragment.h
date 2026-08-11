// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "DamageFragment.generated.h"

/**
 * Provides basic damage information for items that can deal damage
 * (weapons, tools, unarmed, etc.).
 * Actual damage application is handled by the systems that perform the hit.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UDamageFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	/** Type of damage this item deals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UDamageType> DamageType;

	/** Base damage value before any modifiers. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;
};

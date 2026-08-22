// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/DamageType.h"
#include "HarvestableInstance.generated.h"

class UHarvestableDefinition;

/**
 * Long-lived runtime identity / state for one specific harvestable resource.
 * Created with NewObject<UHarvestableInstance>(GetTransientPackage()).
 * Only destroyed when the resource is fully harvested / removed.
 *
 * Created lazily for most world-generated resources (trees, rocks, ore...).
 * Always created immediately for player-planted crops.
 *
 * Fragments write their default runtime values into this object via OnHarvestableInstanceCreated.
 * Additional mutable state (CurrentHealth, growth stage/progress, quality, etc.)
 * can live here directly or be added by future specialized fragments.
 */
UCLASS(BlueprintType)
class WOODCRAFTSURVIVAL_API UHarvestableInstance : public UObject
{
	GENERATED_BODY()

public:
	/** The type definition this instance was created from. Always valid. */
	UPROPERTY(BlueprintReadOnly, Category = "Harvestable")
	TObjectPtr<UHarvestableDefinition> Definition;

	/**
	 * Current health. Set by UHealthHarvestableFragment::OnHarvestableInstanceCreated.
	 * Reduced by ApplyDamage path. When <= 0 the death / yield sequence runs.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 0.f;

	/**
	 * Runtime copy of resistance data. Populated once from UHealthHarvestableFragment.
	 * Status effects may later ++/-- the integer values here.
	 * Positive = more resistant, Negative = more weak. Missing/0 → ×1.0.
	 * Final multiplier = Pow(0.5f, Modifier).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Health|Resistance")
	TMap<TSubclassOf<UDamageType>, int32> DamageModifiers;

	/** Completely ignore these damage types (multiplier 0). Copied from fragment at creation. */
	UPROPERTY(BlueprintReadOnly, Category = "Health|Resistance")
	TArray<TSubclassOf<UDamageType>> DamageImmunities;

	/**
	 * Returns 0 if the damage type is immune, otherwise Pow(0.5f, Modifier).
	 * Missing or 0 modifier → 1.0.
	 */
	float GetDamageMultiplier(TSubclassOf<UDamageType> InDamageType) const;

	// Future: growth stage, progress, water, quality, regrowth timer, etc.
	// can be added here or driven entirely by specialized fragments.
};

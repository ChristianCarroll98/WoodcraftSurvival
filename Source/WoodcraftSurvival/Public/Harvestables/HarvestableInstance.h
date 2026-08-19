// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
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

	// Future: growth stage, progress, water, quality, regrowth timer, etc.
	// can be added here or driven entirely by specialized fragments.
};

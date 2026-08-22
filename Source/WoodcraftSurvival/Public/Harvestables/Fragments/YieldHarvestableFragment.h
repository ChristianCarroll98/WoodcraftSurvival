// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Harvestables/Fragments/HarvestableFragment.h"
#include "YieldHarvestableFragment.generated.h"

class UItemDefinition;
class UHarvestableDefinition;

/**
 * Single deterministic yield entry for MVP.
 */
USTRUCT(BlueprintType)
struct FHarvestableYieldEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yield")
	TSoftObjectPtr<UItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yield", meta = (ClampMin = "0"))
	int32 Count = 1;
};

/**
 * Items (and optional replacement harvestable) produced when this resource is fully destroyed.
 * Pure data – execution lives in AHarvestableActor::HandleDeath.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType, Blueprintable)
class WOODCRAFTSURVIVAL_API UYieldHarvestableFragment : public UHarvestableFragment
{
	GENERATED_BODY()

public:

	/** Items to spawn when the harvestable is destroyed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yield")
	TArray<FHarvestableYieldEntry> Yields;

	/**
	 * Optional leftover harvestable (stump, next boulder stage, etc.).
	 * Spawned at the same transform as the original actor.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yield")
	TSoftObjectPtr<UHarvestableDefinition> ReplacementHarvestable;
};

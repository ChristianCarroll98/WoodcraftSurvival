// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

//#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Harvestables/Fragments/HarvestableFragment.h"
#include "HarvestableDefinition.generated.h"

/**
 * Primary Data Asset that defines a harvestable resource type
 * (OakTree, PineTree, CopperVein, ClayDeposit, Stump_Generic, Carrot, BerryBush, etc.).
 *
 * Pure type data. All capability is added via Fragments.
 * Mesh is stored here for the basic MVP path (same pattern as ItemDefinition).
 */
UCLASS(BlueprintType)
class WOODCRAFTSURVIVAL_API UHarvestableDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Fragments that provide capabilities and data for this harvestable type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Harvestable")
	TArray<TObjectPtr<UHarvestableFragment>> Fragments;

	/** Primary visual mesh for the standing / world form of this harvestable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TSoftObjectPtr<UStaticMesh> PrimaryMesh;

	// Secondary mesh support can be added later if needed for complex trees.
};

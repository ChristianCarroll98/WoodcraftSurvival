// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HarvestableFactorySubsystem.generated.h"

class UHarvestableDefinition;
class UHarvestableInstance;
class AHarvestableActor;

/**
 * Single authority for creating Harvestable Instances and Actors.
 * Access via GetWorld()->GetSubsystem<UHarvestableFactorySubsystem>().
 *
 * Public API (locked):
 *   CreateInstanceFromDefinition
 *   SpawnActorFromDefinition
 *   SpawnActorFromInstance
 *   PromoteToInstance (also callable from the Actor itself)
 */
UCLASS()
class WOODCRAFTSURVIVAL_API UHarvestableFactorySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Creates a long-lived UHarvestableInstance and calls OnInstanceCreated
	 * on every fragment of the Definition. Outer is TransientPackage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Harvestable")
	UHarvestableInstance* CreateInstanceFromDefinition(UHarvestableDefinition* Definition);

	/**
	 * Convenience / world-gen path. Spawns a pure AHarvestableActor that only
	 * references the Definition (no Instance yet – lazy promotion later).
	 */
	UFUNCTION(BlueprintCallable, Category = "Harvestable")
	AHarvestableActor* SpawnActorFromDefinition(UHarvestableDefinition* Definition, const FTransform& Transform);

	/**
	 * Full path. Spawns an AHarvestableActor and initializes it from an existing Instance.
	 */
	UFUNCTION(BlueprintCallable, Category = "Harvestable")
	AHarvestableActor* SpawnActorFromInstance(UHarvestableInstance* Instance, const FTransform& Transform);

	/**
	 * Promotes a pure-Definition Actor to have a real Instance.
	 * Called by the Actor on first meaningful interaction (or can be called externally).
	 */
	UFUNCTION(BlueprintCallable, Category = "Harvestable")
	void PromoteToInstance(AHarvestableActor* Actor);
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>	
#include "ItemFactorySubsystem.generated.h"

class UItemDefinition;
class UItemInstance;
class AItemActor;

/**
 * World Subsystem responsible for creating ItemInstances and spawning ItemActors.
 * Access via: GetWorld()->GetSubsystem<UItemFactorySubsystem>()
 */
UCLASS()
class WOODCRAFTSURVIVAL_API UItemFactorySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Creates a runtime UItemInstance from a Definition (no Actor). */
	UFUNCTION(BlueprintCallable, Category = "Item Factory")
	UItemInstance* CreateItemInstanceFromDefinition(UItemDefinition* Definition);

	/** Convenience: creates Instance + spawns the Actor. */
	UFUNCTION(BlueprintCallable, Category = "Item Factory")
	AItemActor* SpawnItemActorFromDefinition(UItemDefinition* Definition, const FTransform& SpawnTransform);

	/** Core path: spawns an Actor for an already-existing Instance. */
	UFUNCTION(BlueprintCallable, Category = "Item Factory")
	AItemActor* SpawnItemActorFromInstance(UItemInstance* Instance, const FTransform& SpawnTransform);
};

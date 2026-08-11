// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemFactory.generated.h"

class UItemDefinition;
class AItemActor;

/**
 * Factory for creating item instances and spawning them in the world.
 */
UCLASS()
class WOODCRAFTSURVIVAL_API UItemFactory : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Creates a new ItemInstance from a Definition and spawns an AItemActor in the world.
	 * Always creates a single item (StackCount is not used).
	 */
	UFUNCTION(BlueprintCallable, Category = "Item", meta = (WorldContext = "WorldContextObject"))
	static AItemActor* SpawnItemFromDefinition(
		UObject* WorldContextObject,
		const UItemDefinition* Definition,
		FTransform SpawnTransform);
};

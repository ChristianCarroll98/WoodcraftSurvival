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
	 * @param WorldContextObject  Any object in the world (used to get the UWorld).
	 * @param Definition          The item definition to spawn.
	 * @param SpawnTransform      Where to place the actor.
	 * @param StackCount          Starting stack size (clamped by StackableFragment if present).
	 * @return The spawned AItemActor, or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item", meta = (WorldContext = "WorldContextObject"))
	static AItemActor* SpawnItemFromDefinition(
		UObject* WorldContextObject,
		const UItemDefinition* Definition,
		FTransform SpawnTransform,
		int32 StackCount = 1);
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemFactory.h"
#include "Items/ItemDefinition.h"
#include "Items/ItemInstance.h"
#include "Items/ItemActor.h"
#include "Items/Fragments/DurabilityFragment.h"

AItemActor* UItemFactory::SpawnItemFromDefinition(
	UObject* WorldContextObject,
	const UItemDefinition* Definition,
	FTransform SpawnTransform)
{
	if (!WorldContextObject || !Definition)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return nullptr;

	// Create the single runtime instance for this new ItemActor.
	//This is where all mutable state (durability, stack count, etc.) will live.
	UItemInstance* NewInstance = NewObject<UItemInstance>(WorldContextObject);
	NewInstance->UniqueId = FGuid::NewGuid();
	NewInstance->Definition = Definition;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemActor* NewItem = World->SpawnActor<AItemActor>(
			AItemActor::StaticClass(),
			SpawnTransform,
			SpawnParams);
	if (!NewItem) return nullptr;

	// 4. Initialize the actor with the instance (sets mesh, stores the instance, etc.)
	NewItem->InitializeFromInstance(NewInstance);

	return NewItem;
}

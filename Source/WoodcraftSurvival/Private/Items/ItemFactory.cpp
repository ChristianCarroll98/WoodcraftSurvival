// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemFactory.h"
#include "Items/ItemDefinition.h"
#include "Items/ItemInstance.h"
#include "Items/ItemActor.h"
#include "Items/Fragments/DurabilityFragment.h"
#include "Engine/World.h"

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
	if (!World)
	{
		return nullptr;
	}

	// 1. Create a single runtime instance
	UItemInstance* NewInstance = NewObject<UItemInstance>(WorldContextObject);
	NewInstance->Definition = Definition;
	NewInstance->UniqueId = FGuid::NewGuid();

	// 2. Set starting durability if the item has a DurabilityFragment
	if (const UDurabilityFragment* DurabilityFrag = Definition->FindFragment<UDurabilityFragment>())
	{
		NewInstance->CurrentHealth = DurabilityFrag->MaxDurability;
	}

	// 3. Spawn the actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemActor* NewActor = World->SpawnActor<AItemActor>(
		AItemActor::StaticClass(),
		SpawnTransform,
		SpawnParams);

	if (!NewActor)
	{
		return nullptr;
	}

	// 4. Initialize the actor with the instance (sets mesh, stores the instance, etc.)
	NewActor->InitializeFromInstance(NewInstance);

	return NewActor;
}

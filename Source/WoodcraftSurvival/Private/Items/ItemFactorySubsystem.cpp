// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemFactorySubsystem.h"
#include "Items/ItemDefinition.h"
#include "Items/ItemInstance.h"
#include "Items/ItemActor.h"
#include "Items/Fragments/ItemFragment.h"

UItemInstance* UItemFactorySubsystem::CreateItemInstanceFromDefinition(UItemDefinition* Definition)
{
	if (!Definition) return nullptr;

	UItemInstance* NewInstance = NewObject<UItemInstance>(GetTransientPackage());
	NewInstance->UniqueId = FGuid::NewGuid();
	NewInstance->ItemDefinition = Definition;

	for (UItemFragment* Fragment : NewInstance->ItemDefinition->Fragments)
	{
		// Call each fragment's initialization function to set default values
		if (Fragment) Fragment->OnItemInstanceCreated(NewInstance);
	}

	// Add any other default initialization here
	// (fragments will still run later inside InitializeFromInstance)

	return NewInstance;
}

AItemActor* UItemFactorySubsystem::SpawnItemActorFromDefinition(UItemDefinition* Definition, const FTransform& SpawnTransform)
{
	UItemInstance* NewInstance = CreateItemInstanceFromDefinition(Definition);
	if (!NewInstance) return nullptr;

	return SpawnItemActorFromInstance(NewInstance, SpawnTransform);
}

AItemActor* UItemFactorySubsystem::SpawnItemActorFromInstance(UItemInstance* Instance, const FTransform& SpawnTransform)
{
	if (!Instance || !GetWorld()) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemActor* NewItem = GetWorld()->SpawnActor<AItemActor>(
		AItemActor::StaticClass(),
		SpawnTransform,
		SpawnParams);

	if (!NewItem) return nullptr;

	// This is where mesh is set, fragments' OnItemSpawned are called, etc.
	NewItem->InitializeFromInstance(Instance);

	return NewItem;
}

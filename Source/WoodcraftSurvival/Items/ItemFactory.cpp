#include "ItemFactory.h"
#include "ItemDefinition.h"
#include "ItemInstance.h"
#include "ItemActor.h"
#include "Fragments/StackableFragment.h"
#include "Fragments/DurabilityFragment.h"
#include "Engine/World.h"

AItemActor* UItemFactory::SpawnItemFromDefinition(
	UObject* WorldContextObject,
	const UItemDefinition* Definition,
	FTransform SpawnTransform,
	int32 StackCount)
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

	// 1. Create the runtime instance
	UItemInstance* NewInstance = NewObject<UItemInstance>(WorldContextObject);	// outer can be adjusted later
	NewInstance->Definition = Definition;
	NewInstance->UniqueId = FGuid::NewGuid();
	NewInstance->StackCount = FMath::Max(1, StackCount);

	// Clamp stack count if the item has a StackableFragment
	if (const UStackableFragment* StackFrag = Definition->FindFragment<UStackableFragment>())
	{
		NewInstance->StackCount = FMath::Clamp(NewInstance->StackCount, 1, StackFrag->MaxStackSize);
	}
	else
	{
		NewInstance->StackCount = 1;	// non-stackable
	}

	// Set starting durability if the item has a DurabilityFragment
	if (const UDurabilityFragment* DurabilityFrag = Definition->FindFragment<UDurabilityFragment>())
	{
		NewInstance->CurrentDurability = DurabilityFrag->MaxDurability;
	}

	// 2. Spawn the actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemActor* NewActor = World->SpawnActor<AItemActor>(AItemActor::StaticClass(), SpawnTransform, SpawnParams);
	if (!NewActor)
	{
		return nullptr;
	}

	// 3. Initialize the actor with the instance
	NewActor->InitializeFromInstance(NewInstance);

	return NewActor;
}

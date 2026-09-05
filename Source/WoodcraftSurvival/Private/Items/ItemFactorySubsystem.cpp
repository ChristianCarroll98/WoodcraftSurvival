// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemFactorySubsystem.h"
#include "Items/ItemDefinition.h"
#include "Items/ItemInstance.h"
#include "Items/ItemActor.h"
#include "Items/Fragments/ItemFragment.h"

UItemInstance* UItemFactorySubsystem::CreateItemInstanceFromDefinition(const UItemDefinition* Definition)
{
	if (!Definition) return nullptr;

	UItemInstance* NewInstance = NewObject<UItemInstance>(GetTransientPackage());
	//NewInstance->UniqueId = FGuid::NewGuid();
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

AItemActor* UItemFactorySubsystem::SpawnItemActorFromDefinition(
	const UItemDefinition* Definition,
	const FTransform& SpawnTransform,
	bool bJitterRotation)
{
	UItemInstance* NewInstance = CreateItemInstanceFromDefinition(Definition);
	if (!NewInstance) return nullptr;

	return SpawnItemActorFromInstance(NewInstance, SpawnTransform, bJitterRotation);
}

AItemActor* UItemFactorySubsystem::SpawnItemActorFromInstance(
	UItemInstance* Instance,
	const FTransform& SpawnTransform,
	bool bJitterRotation)
{
	if (!Instance || !GetWorld()) return nullptr;

	// Create nice name for the object
	FString BaseName = Instance->ItemDefinition->GetName(); // "DA_IronAxe"
	BaseName.RemoveFromStart(TEXT("DA_"));                  // → "IronAxe"

	// Auto increments BaseName to get unique name for the actor in the editor
	FName UniqueRuntimeName = MakeUniqueObjectName(
		GetWorld(),
		AItemActor::StaticClass(),
		FName(*BaseName)
	);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = UniqueRuntimeName;
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FTransform FinalTransform = SpawnTransform;
	if (bJitterRotation)
	{
		const FRotator SpawnJitter(
			FMath::FRandRange(-25.f, 25.f),
			FMath::FRandRange(-25.f, 25.f),
			FMath::FRandRange(-25.f, 25.f));
		FinalTransform.SetRotation((SpawnJitter.Quaternion() * SpawnTransform.GetRotation()).GetNormalized());
	}

	AItemActor* NewItem = GetWorld()->SpawnActor<AItemActor>(
		AItemActor::StaticClass(),
		FinalTransform,
		SpawnParams);

	if (!NewItem) return nullptr;

#if WITH_EDITOR
	// Make the Outliner show the same clean name
	NewItem->SetActorLabel(UniqueRuntimeName.ToString());
#endif

	// This is where mesh is set, fragments' OnItemSpawned are called, etc.
	NewItem->InitializeFromInstance(Instance);

	return NewItem;
}

void UItemFactorySubsystem::DestroyItemActorAndInstance(AItemActor* Item)
{
	if (!Item) return;

	UItemInstance* Instance = Item->GetItemInstance();
	Item->Destroy();
	if (IsValid(Instance)) Instance->MarkAsGarbage();
}

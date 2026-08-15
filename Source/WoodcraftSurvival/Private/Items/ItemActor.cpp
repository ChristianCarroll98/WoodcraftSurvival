// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemActor.h"
#include "Items/ItemInstance.h"
#include "Items/ItemDefinition.h"
#include "Items/Fragments/ItemFragment.h"
#include <Components/StaticMeshComponent.h>

AItemActor::AItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create the mesh component and make it the root
	PrimaryMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimaryMeshComponent"));
	SetRootComponent(PrimaryMeshComponent);
	PrimaryMeshComponent->SetCollisionProfileName(TEXT("Item"));
}

UItemInstance* AItemActor::GetItemInstance() const
{
	return ItemInstance;
}

void AItemActor::InitializeFromInstance(UItemInstance* Instance)
{
	if (!Instance || !Instance->ItemDefinition) return;

	ItemInstance = Instance;

	// Load and apply the primary mesh from the definition
	if (UStaticMesh* Mesh = Instance->ItemDefinition->PrimaryMesh.LoadSynchronous())
	{
		PrimaryMeshComponent->SetStaticMesh(Mesh);
	}
	
	// Secondary mesh (optional)
	if (!Instance->ItemDefinition->SecondaryMesh.IsNull())
	{
		if (UStaticMesh* SecondaryMesh = Instance->ItemDefinition->SecondaryMesh.LoadSynchronous())
		{
			SecondaryMeshComponent = NewObject<UStaticMeshComponent>(this, TEXT("SecondaryMeshComponent"));

			SecondaryMeshComponent->SetStaticMesh(SecondaryMesh);
			SecondaryMeshComponent->SetRelativeTransform(Instance->ItemDefinition->SecondaryRelativeTransform);
			SecondaryMeshComponent->SetCollisionProfileName(TEXT("Item"));

			SecondaryMeshComponent->SetupAttachment(PrimaryMeshComponent);
			SecondaryMeshComponent->RegisterComponent();
		}
	}

	SetActorLabel(*(Instance->ItemDefinition->DisplayName.ToString() + "_" + Instance->UniqueId.ToString()));

	for (UItemFragment* Fragment : Instance->ItemDefinition->Fragments)
	{
		// Call each fragment's OnItemSpawned function on this item to allow them to initialize the item actor
		if (Fragment) Fragment->OnItemSpawned(this);
	}
}

UStaticMeshComponent* AItemActor::GetItemPrimaryMesh() const
{
	return PrimaryMeshComponent;
}

UStaticMeshComponent* AItemActor::GetItemSecondaryMesh() const
{
	return SecondaryMeshComponent;
}

FTransform AItemActor::GetSecondaryRelativeTransform() const
{
	return ItemInstance->ItemDefinition->SecondaryRelativeTransform;
}

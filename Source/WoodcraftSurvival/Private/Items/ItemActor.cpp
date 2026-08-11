// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemActor.h"
#include "Items/ItemInstance.h"
#include "Items/ItemDefinition.h"
#include "Components/StaticMeshComponent.h"

AItemActor::AItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create the mesh component and make it the root
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	// Reasonable defaults for a world item
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));	// or your preferred profile
}

UItemInstance* AItemActor::GetItemInstance() const
{
	return ItemInstance;
}

void AItemActor::InitializeFromInstance(UItemInstance* Instance)
{
	if (!Instance || !Instance->Definition)
	{
		return;
	}

	ItemInstance = Instance;

	// Load and apply the mesh from the definition
	if (UStaticMesh* Mesh = Instance->Definition->StaticMesh.LoadSynchronous())
	{
		MeshComponent->SetStaticMesh(Mesh);
	}

	// You can add more initialization here later (collision, materials, etc.)
}

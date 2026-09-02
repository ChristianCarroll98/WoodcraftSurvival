// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Harvestables/Fragments/CollisionHarvestableFragment.h"
#include "Harvestables/HarvestableActor.h"
#include "Core/WoodcraftTypes.h"
#include "Components/StaticMeshComponent.h"

void UCollisionHarvestableFragment::OnHarvestableSpawned(AHarvestableActor* Actor)
{
	if (!Actor) return;
	if (Mode != EHarvestableCollisionMode::WalkThrough) return;

	UStaticMeshComponent* Mesh = Actor->GetPrimaryMeshComponent();
	if (!Mesh) return;

	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(COLLISION_PLAYER, ECR_Ignore);
}

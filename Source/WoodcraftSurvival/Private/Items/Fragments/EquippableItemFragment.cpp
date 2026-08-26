// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/EquippableItemFragment.h"
#include "Items/ItemActor.h"
#include "Core/WoodcraftTypes.h"

void UEquippableItemFragment::OnItemSpawned(AItemActor* ItemActor)
{	
	if (!ItemActor) return;

	if (UStaticMeshComponent* Primary = ItemActor->GetItemPrimaryMesh())
	{
		Primary->SetCollisionResponseToChannel(TRACE_EQUIPPABLE, ECollisionResponse::ECR_Block);
	}

	if (UStaticMeshComponent* Secondary = ItemActor->GetItemSecondaryMesh())
	{
		Secondary->SetCollisionResponseToChannel(TRACE_EQUIPPABLE, ECollisionResponse::ECR_Block);
	}
}

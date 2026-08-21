// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/DamageItemFragment.h"
#include "Items/ItemActor.h"

void UDamageItemFragment::OnItemSpawned(AItemActor* ItemActor)
{
	if (!ItemActor) return;

	if (UStaticMeshComponent* Primary = ItemActor->GetItemPrimaryMesh())
	{
		ItemActor->EnableCollisionDamage(Primary);
	}

	if (UStaticMeshComponent* Secondary = ItemActor->GetItemSecondaryMesh())
	{
		ItemActor->EnableCollisionDamage(Secondary);
	}
}

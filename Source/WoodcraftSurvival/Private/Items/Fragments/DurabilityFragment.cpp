// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/DurabilityFragment.h"
#include "Items/ItemActor.h"
#include "Items/ItemInstance.h"

void UDurabilityFragment::OnItemSpawned(AItemActor* ItemActor)
{
	ItemActor->GetItemInstance()->CurrentHealth = MaxDurability;
}
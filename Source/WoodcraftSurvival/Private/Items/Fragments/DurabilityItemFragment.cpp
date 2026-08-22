// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/DurabilityItemFragment.h"
#include "Items/ItemInstance.h"

void UDurabilityItemFragment::OnItemInstanceCreated(UItemInstance* Instance)
{
	Instance->CurrentHealth = MaxDurability;
}

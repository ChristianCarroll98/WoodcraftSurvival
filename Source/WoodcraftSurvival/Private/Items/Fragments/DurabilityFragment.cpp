// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/DurabilityFragment.h"
#include "Items/ItemInstance.h"

void UDurabilityFragment::OnItemInstanceCreated(UItemInstance* Instance)
{
	Instance->CurrentHealth = MaxDurability;
}
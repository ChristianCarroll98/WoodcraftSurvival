// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Harvestables/Fragments/HealthHarvestableFragment.h"
#include "Harvestables/HarvestableInstance.h"

void UHealthHarvestableFragment::OnHarvestableInstanceCreated(UHarvestableInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	Instance->CurrentHealth = MaxHealth;
	Instance->DamageModifiers = DamageModifiers;
	Instance->DamageImmunities = DamageImmunities;
}

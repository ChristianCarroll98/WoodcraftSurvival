// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "HealthHarvestableFragment.h"
#include "HarvestableInstance.h"

void UHealthHarvestableFragment::OnInstanceCreated(UHarvestableInstance* Instance)
{
	if (Instance)
	{
		Instance->CurrentHealth = MaxHealth;
	}
}

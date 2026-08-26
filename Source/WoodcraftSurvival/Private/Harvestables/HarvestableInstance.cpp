// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Harvestables/HarvestableInstance.h"

float UHarvestableInstance::GetDamageMultiplier(TSubclassOf<UDamageType> InDamageType) const
{
	if (!InDamageType)
	{
		return 1.f;
	}

	if (DamageImmunities.Contains(InDamageType))
	{
		return 0.f;
	}

	if (const int32* Mod = DamageModifiers.Find(InDamageType))
	{
		return FMath::Pow(0.5f, static_cast<float>(*Mod));
	}

	return 1.f;
}

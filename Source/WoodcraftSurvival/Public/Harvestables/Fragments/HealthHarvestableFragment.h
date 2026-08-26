// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HarvestableFragment.h"
#include "GameFramework/DamageType.h"
#include "HealthHarvestableFragment.generated.h"

/**
 * Provides MaxHealth and resistance defaults.
 * OnHarvestableInstanceCreated copies CurrentHealth + resistance data onto the long-lived Instance.
 * Fragments remain pure data; all runtime mutation lives on the Instance.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType, Blueprintable)
class WOODCRAFTSURVIVAL_API UHealthHarvestableFragment : public UHarvestableFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0"))
	float MaxHealth = 100.f;

	/**
	 * Positive = more resistant, Negative = more weak.
	 * Missing or 0 → multiplier 1.0.
	 * Final multiplier = Pow(0.5f, Modifier).
	 * Copied onto the Instance at creation; status effects later modify the Instance copy.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health|Resistance")
	TMap<TSubclassOf<UDamageType>, int32> DamageModifiers;

	/**
	 * Completely ignore these damage types (multiplier 0).
	 * Copied onto the Instance at creation.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health|Resistance")
	TArray<TSubclassOf<UDamageType>> DamageImmunities;

	virtual void OnHarvestableInstanceCreated(UHarvestableInstance* Instance) override;
};

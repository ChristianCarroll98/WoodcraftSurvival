// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HarvestableFragment.h"
#include "HealthHarvestableFragment.generated.h"

/**
 * Provides MaxHealth and initializes CurrentHealth on the Instance.
 * First concrete fragment for the Tree MVP.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType, Blueprintable)
class WOODCRAFTSURVIVAL_API UHealthHarvestableFragment : public UHarvestableFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0"))
	float MaxHealth = 100.f;

	virtual void OnInstanceCreated(UHarvestableInstance* Instance) override;
};

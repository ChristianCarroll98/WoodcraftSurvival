// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HarvestableFragment.generated.h"

class UHarvestableInstance;
class AHarvestableActor;

/**
 * Abstract base for all harvestable capability fragments.
 * Primarily pure data (EditDefaultsOnly). 
 * Two lightweight virtuals that mirror the Item system:
 *   - OnInstanceCreated  → one-time runtime defaults on the long-lived Instance
 *   - OnHarvestableSpawned → actor-side setup every time an Actor is created/initialized
 *
 * Concrete fragments follow the naming convention:
 *   UHealthHarvestableFragment, UYieldHarvestableFragment, UCropHarvestableFragment, etc.
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, BlueprintType, Blueprintable)
class WOODCRAFTSURVIVAL_API UHarvestableFragment : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Called only once when a new UHarvestableInstance is first created
	 * (via Factory::CreateInstanceFromDefinition or PromoteToInstance).
	 * Use for default runtime values (CurrentHealth = MaxHealth, growth progress = 0, etc.).
	 * Never perform actor-side work here.
	 */
	virtual void OnInstanceCreated(UHarvestableInstance* Instance) {}

	/**
	 * Called every time an AHarvestableActor is spawned / initialized
	 * (both pure-Definition and Instance paths).
	 * Use only for actor-side setup (collision responses, mesh tweaks, components).
	 * Never write default values into the Instance here.
	 */
	virtual void OnHarvestableSpawned(AHarvestableActor* Actor) {}
};

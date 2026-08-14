// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "ItemFragment.generated.h"

class AItemActor;
class UItemInstance;

/**
 * Base class for all item fragments.
 * Fragments are pure data. They describe a capability an item can have.
 * Never put gameplay logic here — that belongs in the components that read the fragments.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UItemFragment : public UObject
{
	GENERATED_BODY()

public:

	/** Called only once when a new UItemInstance is first created from a Definition.
	 *  Use this to set default runtime values (CurrentDurability, etc.). */
	virtual void OnItemInstanceCreated(UItemInstance* Instance) {}

	/** Called every time an AItemActor is spawned/initialized for an Instance.
	 *  Use this only for actor-side setup (collision, components, mesh tweaks, etc.).
	 *  Never write default values into the Instance here. */
	virtual void OnItemSpawned(AItemActor* ItemActor) {}

	// Concrete fragments will add their own data (MaxStackSize, BurnTime, etc.).
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "ItemFragment.h"
#include "DamageItemFragment.generated.h"

/**
 * Pure marker fragment that enables collision hit events on an item.
 * Presence of this fragment causes AItemActor to bind OnComponentHit on
 * Primary and Secondary meshes and to apply damage on valid impacts.
 *
 * DamageType is derived at hit time from the named collision primitive
 * (Blunt / Slash / Pierce / aliases). Amount is derived from impulse magnitude.
 * No BaseDamage or DamageType data is stored on the fragment.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UDamageItemFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	/** Enables hit events + binds the actor's OnItemMeshHit handler on Primary and Secondary meshes. */
	virtual void OnItemSpawned(AItemActor* ItemActor) override;
};

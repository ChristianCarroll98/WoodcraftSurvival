// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Items/Fragments/ItemFragment.h"
#include "PhysicsFragment.generated.h"

class AItemActor;

/**
 * Marks an item as a physics object that can move, collide, and be affected by forces in the world.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UPhysicsFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	void OnItemSpawned(AItemActor* ItemActor);
};

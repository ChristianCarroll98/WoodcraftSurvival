// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/Fragments/ItemFragment.h"
#include "Core/WoodcraftTypes.h"
#include "EquippableFragment.generated.h"

/**
 * Data required to hold / equip an item in the hands.
 * Used by HeldItemComponent for attachment, two-handed logic, and later poses / IK.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UEquippableFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	void OnItemSpawned(AItemActor* ItemActor);

	/** If true, the item requires both hands. Secondary hand is derived automatically. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
	bool bTwoHanded = false;
};

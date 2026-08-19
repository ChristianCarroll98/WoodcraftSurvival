// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Items/Fragments/ItemFragment.h"
#include "EquippableItemFragment.generated.h"

class AItemActor;

/**
 * Data required to hold / equip an item in the hands.
 * Used by HeldItemsComponent for attachment, two-handed logic, and later poses / IK.
 */
UCLASS(EditInlineNew, DefaultToInstanced)
class WOODCRAFTSURVIVAL_API UEquippableItemFragment : public UItemFragment
{
	GENERATED_BODY()

public:

	void OnItemSpawned(AItemActor* ItemActor);

	/** If true, the item requires both hands. Secondary hand is derived automatically. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
	bool bTwoHanded = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable|Animation")
	TSoftObjectPtr<UAnimSequence> NeutralPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable|Animation")
	TSoftObjectPtr<UAnimSequence> ExtendedPose;
};

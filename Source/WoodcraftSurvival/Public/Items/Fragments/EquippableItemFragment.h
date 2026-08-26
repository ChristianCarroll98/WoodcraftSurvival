// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Items/Fragments/ItemFragment.h"
#include "Core/WoodcraftTypes.h"
#include "EquippableItemFragment.generated.h"

class AItemActor;

/**
 * Data required to hold / equip an item in the hands.
 * Used by HeldItemsComponent for attachment, two-handed logic, hold poses, and strike mode.
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

	/**
	 * Preferred strike axis for incidence checks and procedural orientation.
	 * Shape name still decides the candidate damage type; this decides which local axis
	 * must be aligned with velocity to keep Slash / Pierce instead of forcing Blunt.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable|Strike")
	EItemStrikeMode StrikeMode = EItemStrikeMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable|Animation")
	TSoftObjectPtr<UAnimSequence> NeutralPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable|Animation")
	TSoftObjectPtr<UAnimSequence> ExtendedPose;
};

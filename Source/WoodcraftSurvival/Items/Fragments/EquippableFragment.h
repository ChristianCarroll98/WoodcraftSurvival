#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
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

	/** If true, the item requires both hands. Secondary hand is derived automatically. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
	bool bTwoHanded = false;

	/** Socket on the hand mesh to attach to when held in the primary hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
	FName PrimaryHandSocket = TEXT("hand_r");

	/** Optional socket for the secondary hand when the item is two-handed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable", meta = (EditCondition = "bTwoHanded"))
	FName SecondaryHandSocket = TEXT("hand_l");

	/** Local transform applied relative to the primary hand socket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
	FTransform PrimaryHandTransform = FTransform::Identity;
};

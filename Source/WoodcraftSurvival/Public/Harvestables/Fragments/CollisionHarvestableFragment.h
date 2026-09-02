// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "HarvestableFragment.h"
#include "CollisionHarvestableFragment.generated.h"

class AHarvestableActor;

UENUM(BlueprintType)
enum class EHarvestableCollisionMode : uint8
{
	/** Leave HarvestableProfile responses (trees, boulders, stumps, ore). */
	Solid		UMETA(DisplayName = "Solid"),

	/** Ignore Pawn + Player so the player walks through. Item stays Block for swing hits. */
	WalkThrough	UMETA(DisplayName = "Walk Through")
};

/**
 * Optional collision override applied after HarvestableProfile.
 * Trees / rocks omit this fragment. Plants and later crops add it and set WalkThrough.
 * Does not change Collision Enabled or the Item response — OnComponentHit needs those.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType, Blueprintable)
class WOODCRAFTSURVIVAL_API UCollisionHarvestableFragment : public UHarvestableFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	EHarvestableCollisionMode Mode = EHarvestableCollisionMode::WalkThrough;

	virtual void OnHarvestableSpawned(AHarvestableActor* Actor) override;
};

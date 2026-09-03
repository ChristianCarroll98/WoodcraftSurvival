// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Core/WoodcraftTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CraftingRecipeDefinition.generated.h"

class AItemActor;
class UCraftingMinigameDefinition;
class UCraftingRecipeDefinition;
class UItemDefinition;

/**
 * One input socket on a recipe.
 * One instance per slot. There is no count field.
 */
USTRUCT(BlueprintType)
struct FCraftingSlot
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot")
	ECraftingSlotRole Role = ECraftingSlotRole::Ingredient;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot")
	ECraftingSlotMatchMode Match = ECraftingSlotMatchMode::ExactDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot", meta = (EditCondition = "Match == ECraftingSlotMatchMode::ExactDefinition"))
	TObjectPtr<const UItemDefinition> ExactDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot", meta = (EditCondition = "Match == ECraftingSlotMatchMode::RequiredTag"))
	FGameplayTag RequiredTag;

	/** True = must bind to a hand. Tools are always held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot")
	bool bMustBeHeld = true;

	/** Factory-destroy this bound instance on success. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot")
	bool bConsumed = true;

	/** Applied to the bound instance on success. 0 = none. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot", meta = (ClampMin = "0.0"))
	float DurabilityCost = 0.f;
};

/** One spawned result of a successful craft. */
USTRUCT(BlueprintType)
struct FCraftingOutput
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Output")
	TObjectPtr<const UItemDefinition> ItemDefinition;

	/** At most one output per recipe should be true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Output")
	bool bAutoEquip = true;
};

/** Hands plus optional station contents at match time. */
USTRUCT(BlueprintType)
struct FCraftingSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AItemActor> LeftActor;

	UPROPERTY()
	bool bLeftUnarmed = true;

	UPROPERTY()
	bool bLeftExtended = false;

	UPROPERTY()
	bool bLeftStacked = false;

	UPROPERTY()
	TObjectPtr<AItemActor> RightActor;

	UPROPERTY()
	bool bRightUnarmed = true;

	UPROPERTY()
	bool bRightExtended = false;

	UPROPERTY()
	bool bRightStacked = false;

	/** Unset = no station in context. A2 snapshots leave this empty. */
	UPROPERTY()
	TSoftObjectPtr<UPrimaryDataAsset> Station;

	UPROPERTY()
	TObjectPtr<AItemActor> StationActor;
};

/** One recipe slot bound to a hand or station item actor. */
USTRUCT(BlueprintType)
struct FCraftingSlotBinding
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY()
	EHand Hand = EHand::None;

	UPROPERTY()
	bool bStation = false;

	UPROPERTY()
	TObjectPtr<AItemActor> Actor;
};

/** A recipe that matches the current snapshot, with slot bindings. */
USTRUCT(BlueprintType)
struct FCraftingMatch
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UCraftingRecipeDefinition> Recipe;

	UPROPERTY()
	TArray<FCraftingSlotBinding> Bindings;
};

/**
 * Static definition of one craft (DA_Recipe_SharpenedStone, DA_Recipe_Cordage, …).
 */
UCLASS(BlueprintType)
class WOODCRAFTSURVIVAL_API UCraftingRecipeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UCraftingRecipeDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<UTexture2D> Icon;

	/**
	 * Optional station identity. Unset = hands craft.
	 * UStructureDefinition when Structures exist; UPrimaryDataAsset until then.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TSoftObjectPtr<UPrimaryDataAsset> Station;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TArray<FCraftingSlot> Slots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TArray<FCraftingOutput> Outputs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TObjectPtr<UCraftingMinigameDefinition> Minigame;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Returns every recipe in Recipes that can bind to Snapshot. */
	static TArray<FCraftingMatch> FindMatches(
		const FCraftingSnapshot& Snapshot,
		const TArray<UCraftingRecipeDefinition*>& Recipes);
};

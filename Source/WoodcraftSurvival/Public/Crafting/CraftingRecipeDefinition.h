// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Core/WoodcraftTypes.h"
#include "Crafting/Movements/CraftMovement.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CraftingRecipeDefinition.generated.h"

class AItemActor;
class UAnimMontage;
class UCraftingRecipeDefinition;
class UItemDefinition;
class USkeletalMesh;

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

/**
 * One presentation SKM for a stage.
 * Montage is optional. No montage = spawn pose / first frame after intro.
 */
USTRUCT(BlueprintType)
struct FCraftPresentation
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UAnimMontage> Montage;

	/**
	 * ArmsPivot = Identity on AnimRef / ArmsPivot.
	 * Primary / Secondary = that bound item’s primary-mesh world transform.
	 * Station = slot socket Identity.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	ECraftPresentationAnchor Anchor = ECraftPresentationAnchor::ArmsPivot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	ECraftingMorphSampleMode MorphSampleMode = ECraftingMorphSampleMode::Lerp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TArray<FName> MorphChannelNames;
};

/**
 * One minigame stage. Grind / Twist / Strip author one row. Tie authors three.
 */
USTRUCT(BlueprintType)
struct FCraftStage
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Stage")
	TObjectPtr<UCraftMovement> Move;

	/** FPArms TwoHanded clip. Intro at the front, IntroDone notify, then interactive tail. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	TArray<FCraftPresentation> Presentations;

	/** Stage meter target. 1 = one second at the move’s cap rate. Progress = AccumulatedWork / WorkRequired. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "0.01"))
	float WorkRequired = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	bool bGripIK_Left = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	bool bGripIK_Right = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	bool bHideLeft = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	bool bHideRight = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	bool bHideStation = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	ECraftHintGesture Hint = ECraftHintGesture::None;
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

	/** Length ≥ 1. Grind / Twist / Strip author one stage. Tie authors three. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TArray<FCraftStage> Stages;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	int32 GetStageCount() const { return Stages.Num(); }

	const FCraftStage* GetStage(int32 StageIndex) const
	{
		return Stages.IsValidIndex(StageIndex) ? &Stages[StageIndex] : nullptr;
	}

	/**
	 * Finds the movement module on the requested stage if it is class T.
	 * Example: const UGrindActiveCraftMovement* Grind = Recipe->FindMove<UGrindActiveCraftMovement>(Phase);
	 */
	template<typename T>
	const T* FindMove(int32 StageIndex) const
	{
		const FCraftStage* Stage = GetStage(StageIndex);
		if (!Stage) return nullptr;
		return Cast<T>(Stage->Move.Get());
	}

	/** Returns every recipe in Recipes that can bind to Snapshot. */
	static TArray<FCraftingMatch> FindMatches(
		const FCraftingSnapshot& Snapshot,
		const TArray<UCraftingRecipeDefinition*>& Recipes);
};

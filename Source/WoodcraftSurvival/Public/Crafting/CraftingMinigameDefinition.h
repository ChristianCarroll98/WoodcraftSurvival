// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Core/WoodcraftTypes.h"
#include "Crafting/Movements/CraftMovement.h"
#include "Engine/DataAsset.h"
#include "CraftingMinigameDefinition.generated.h"

class UAnimMontage;
class USkeletalMesh;

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

/**
 * Shared look, camera, and an ordered stage list.
 * Gesture numbers live on the instanced UCraftMovement on each stage.
 */
UCLASS(BlueprintType)
class WOODCRAFTSURVIVAL_API UCraftingMinigameDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UCraftingMinigameDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame")
	TArray<FCraftStage> Stages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	ECraftingAppearanceMode AppearanceMode = ECraftingAppearanceMode::LiveMeshes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	ECraftingMorphSampleMode MorphSampleMode = ECraftingMorphSampleMode::Lerp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	FTransform PresentationOffset = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	TArray<FName> MorphChannelNames;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Camera")
	float CraftViewPitchOffset = -30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Camera")
	float CraftViewYawOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Camera", meta = (ClampMin = "0.0"))
	float CraftViewBlendTime = 0.25f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	int32 GetStageCount() const { return Stages.Num(); }

	const FCraftStage* GetStage(int32 StageIndex) const
	{
		return Stages.IsValidIndex(StageIndex) ? &Stages[StageIndex] : nullptr;
	}

	/**
	 * Finds the movement module on the requested stage if it is class T.
	 * Example: const UGrindActiveCraftMovement* Grind = Minigame->FindMove<UGrindActiveCraftMovement>(Phase);
	 */
	template<typename T>
	const T* FindMove(int32 StageIndex) const
	{
		const FCraftStage* Stage = GetStage(StageIndex);
		if (!Stage) return nullptr;
		return Cast<T>(Stage->Move.Get());
	}
};

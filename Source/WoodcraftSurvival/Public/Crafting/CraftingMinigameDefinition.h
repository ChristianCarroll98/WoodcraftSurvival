// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Crafting/Movements/CraftMovement.h"
#include "Engine/DataAsset.h"
#include "CraftingMinigameDefinition.generated.h"

class USkeletalMesh;

/**
 * Shared look, camera, and presentation data for one minigame.
 * Gesture numbers live on the instanced UCraftMovement.
 */
UCLASS(BlueprintType)
class WOODCRAFTSURVIVAL_API UCraftingMinigameDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UCraftingMinigameDefinition();

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Minigame")
	TObjectPtr<UCraftMovement> Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	ECraftingAppearanceMode AppearanceMode = ECraftingAppearanceMode::LiveMeshes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	ECraftingMorphSampleMode MorphSampleMode = ECraftingMorphSampleMode::Lerp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	TSoftObjectPtr<USkeletalMesh> PresentationMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	bool bHideWorkpiece = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minigame|Appearance")
	bool bHideTool = false;

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

	/**
	 * Finds the movement module if it is the requested class.
	 * Example: const UGrindActiveCraftMovement* Grind = Minigame->FindMove<UGrindActiveCraftMovement>();
	 */
	template<typename T>
	const T* FindMove() const
	{
		return Cast<T>(Move.Get());
	}
};

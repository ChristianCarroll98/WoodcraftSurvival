// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Crafting/CraftingRecipeDefinition.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UHeldItemsComponent;

/**
 * Owns the A2 recipe table, snapshot, and match prompt.
 * Session start / cancel / commit land in later cards.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class WOODCRAFTSURVIVAL_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCraftingComponent();

	/** A2 / MVP recipe table. Assign DA_Recipe_SharpenedStone and DA_Recipe_Cordage on the player BP. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TArray<TSoftObjectPtr<UCraftingRecipeDefinition>> RecipeAssets;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	void ResolveRecipeAssets();
	void HandleHeldItemsChanged();
	void RebuildSnapshot();
	void RefreshMatches();
	void UpdateDebugPrompt() const;
	void FillHandSnapshot(EHand Hand);

	UPROPERTY()
	TObjectPtr<UHeldItemsComponent> HeldItems;

	UPROPERTY()
	TArray<TObjectPtr<UCraftingRecipeDefinition>> LoadedRecipes;

	FCraftingSnapshot CurrentSnapshot;
	TArray<FCraftingMatch> CurrentMatches;
	int32 SelectedMatchIndex = 0;
};

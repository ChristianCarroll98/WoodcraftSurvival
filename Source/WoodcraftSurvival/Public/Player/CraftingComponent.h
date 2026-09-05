// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Crafting/CraftingRecipeDefinition.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UHeldItemsComponent;
class UInputMappingContext;
class USceneComponent;

/**
 * Frozen craft while a minigame is live.
 * Created on E, cleared on R, success, fail, or NotifyOwnerDamaged().
 */
USTRUCT()
struct FCraftingSession
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UCraftingRecipeDefinition> Recipe;

	UPROPERTY()
	TArray<FCraftingSlotBinding> Bindings;

	float Progress = 0.f;
	int32 Phase = 0;
	bool bEngage = false;
	EHand EngageHand = EHand::Right;

	UPROPERTY()
	TObjectPtr<AActor> Presentation;
};

/**
 * Owns the recipe table, snapshot, match prompt, and the live craft session.
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

	/** Gameplay IMC removed for the session and restored on teardown. Assign the same asset the player uses to move / look / Q. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Input")
	TObjectPtr<UInputMappingContext> GameplayMappingContext;

	/** Session-only IMC. R cancel plus LMB / RMB engage. No move, look, Q, or wheel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Input")
	TObjectPtr<UInputMappingContext> CraftingMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Input")
	int32 GameplayIMCPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Input")
	int32 CraftingIMCPriority = 1;

	/** Shared FPCamera relative transform for every hands / ground craft. Station crafts use the station’s pose later. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Camera")
	FTransform GroundCraftCameraTransform = FTransform(FRotator(-50.f, 0.f, 0.f), FVector(25.f, 0.f, 65.f), FVector::OneVector);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool TryStartCraft();

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void CancelCraft();

	/** Wheel down = +1 (next match). No-op during a session or when N < 2. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void CycleCraftMatch(int32 Delta);

	/** Player BP reports both buttons. Ignored when Hand is not Session.EngageHand. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SetCraftEngage(EHand Hand, bool bPressed);

	/** Same teardown as CancelCraft. Wire when the player can take damage. */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void NotifyOwnerDamaged();

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsSessionActive() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	EHand GetEngageHand() const;

private:

	void ResolveRecipeAssets();
	void HandleHeldItemsChanged();
	void RebuildSnapshot();
	void RefreshMatches();
	void UpdateDebugPrompt() const;
	void FillHandSnapshot(EHand Hand);

	bool CanStartCraft() const;
	EHand ResolveEngageHand(const FCraftingMatch& Match) const;
	void EndSession();
	void ApplyGroundCraftView();
	void RestoreGroundCraftView();
	USceneComponent* FindFirstPersonCamera() const;
	void PushCraftingIMC();
	void PopCraftingIMC();
	class UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	UPROPERTY()
	TObjectPtr<UHeldItemsComponent> HeldItems;

	UPROPERTY()
	TArray<TObjectPtr<UCraftingRecipeDefinition>> LoadedRecipes;

	UPROPERTY()
	FCraftingSession Session;

	FCraftingSnapshot CurrentSnapshot;
	TArray<FCraftingMatch> CurrentMatches;
	int32 SelectedMatchIndex = 0;
	bool bCraftingIMCPushed = false;
	bool bCraftViewApplied = false;
	FTransform CachedCameraRelativeTransform = FTransform::Identity;
	FRotator CachedControlRotation = FRotator::ZeroRotator;

	/** Default working hand until handedness settings exist. */
	EHand DefaultCraftHand = EHand::Right;
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/CraftingComponent.h"
#include "Items/ItemActor.h"
#include "Player/HeldItemsComponent.h"

namespace
{
	constexpr int32 CraftPromptMessageId = 8201;
}

UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveRecipeAssets();

	HeldItems = GetOwner() ? GetOwner()->FindComponentByClass<UHeldItemsComponent>() : nullptr;
	if (HeldItems)
	{
		HeldItems->OnHeldItemsChanged.AddUObject(this, &UCraftingComponent::HandleHeldItemsChanged);
	}

	HandleHeldItemsChanged();
}

void UCraftingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HeldItems)
	{
		HeldItems->OnHeldItemsChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UCraftingComponent::HandleHeldItemsChanged()
{
	if (LoadedRecipes.Num() == 0 && RecipeAssets.Num() > 0)
	{
		ResolveRecipeAssets();
	}

	RebuildSnapshot();
	RefreshMatches();
	UpdateDebugPrompt();
}

void UCraftingComponent::ResolveRecipeAssets()
{
	LoadedRecipes.Reset();
	for (const TSoftObjectPtr<UCraftingRecipeDefinition>& RecipeAsset : RecipeAssets)
	{
		if (UCraftingRecipeDefinition* Recipe = RecipeAsset.LoadSynchronous())
		{
			LoadedRecipes.Add(Recipe);
		}
	}
}

void UCraftingComponent::FillHandSnapshot(EHand Hand)
{
	AItemActor* ItemActor = HeldItems ? HeldItems->GetHeldItem(Hand) : nullptr;
	const bool bUnarmed = HeldItems && HeldItems->GetIsUnarmed(Hand);
	const bool bExtended = HeldItems && HeldItems->GetIsExtended(Hand);

	if (Hand == EHand::Left)
	{
		CurrentSnapshot.LeftActor = ItemActor;
		CurrentSnapshot.bLeftUnarmed = bUnarmed;
		CurrentSnapshot.bLeftExtended = bExtended;
		CurrentSnapshot.bLeftStacked = false;
	}
	else if (Hand == EHand::Right)
	{
		CurrentSnapshot.RightActor = ItemActor;
		CurrentSnapshot.bRightUnarmed = bUnarmed;
		CurrentSnapshot.bRightExtended = bExtended;
		CurrentSnapshot.bRightStacked = false;
	}
}

void UCraftingComponent::RebuildSnapshot()
{
	FillHandSnapshot(EHand::Left);
	FillHandSnapshot(EHand::Right);

	CurrentSnapshot.Station.Reset();
	CurrentSnapshot.StationActor = nullptr;
}

void UCraftingComponent::RefreshMatches()
{
	TArray<UCraftingRecipeDefinition*> Recipes;
	Recipes.Reserve(LoadedRecipes.Num());
	for (UCraftingRecipeDefinition* Recipe : LoadedRecipes)
	{
		if (Recipe) Recipes.Add(Recipe);
	}

	CurrentMatches = UCraftingRecipeDefinition::FindMatches(CurrentSnapshot, Recipes);

	if (CurrentMatches.Num() == 0)
	{
		SelectedMatchIndex = 0;
		return;
	}

	if (!CurrentMatches.IsValidIndex(SelectedMatchIndex))
	{
		SelectedMatchIndex = 0;
	}
}

void UCraftingComponent::UpdateDebugPrompt() const
{
	if (!GEngine) return;

	const bool bBothNeutral = !CurrentSnapshot.bLeftExtended && !CurrentSnapshot.bRightExtended;
	const bool bShowPrompt = bBothNeutral && CurrentMatches.IsValidIndex(SelectedMatchIndex);
	if (!bShowPrompt)
	{
		GEngine->AddOnScreenDebugMessage(CraftPromptMessageId, 0.f, FColor::Cyan, FString());
		return;
	}

	const FCraftingMatch& Match = CurrentMatches[SelectedMatchIndex];
	const FText DisplayName = Match.Recipe ? Match.Recipe->DisplayName : FText();
	const FString CraftName = DisplayName.IsEmpty()
		? (Match.Recipe ? Match.Recipe->GetName() : TEXT("?"))
		: DisplayName.ToString();

	GEngine->AddOnScreenDebugMessage(
		CraftPromptMessageId,
		10000.f,
		FColor::Cyan,
		FString::Printf(TEXT("[E] Craft %s"), *CraftName));
}

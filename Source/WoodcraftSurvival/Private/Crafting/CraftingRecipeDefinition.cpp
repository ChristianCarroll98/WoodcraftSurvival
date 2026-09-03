// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Crafting/CraftingRecipeDefinition.h"
#include "Items/ItemDefinition.h"
#include "Items/ItemInstance.h"

UCraftingRecipeDefinition::UCraftingRecipeDefinition()
{
	DisplayName = FText::FromString(TEXT("Craft"));
}

FPrimaryAssetId UCraftingRecipeDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CraftingRecipe"), GetFName());
}

static const UItemDefinition* GetSnapshotDefinition(const FCraftingSnapshot& Snapshot, EHand Hand, bool bStation)
{
	if (bStation) return Snapshot.StationInstance ? Snapshot.StationInstance->ItemDefinition.Get() : nullptr;
	if (Hand == EHand::Left) return Snapshot.LeftDefinition;
	if (Hand == EHand::Right) return Snapshot.RightDefinition;
	return nullptr;
}

static AItemActor* GetSnapshotActor(const FCraftingSnapshot& Snapshot, EHand Hand, bool bStation)
{
	if (bStation) return Snapshot.StationActor;
	if (Hand == EHand::Left) return Snapshot.LeftActor;
	if (Hand == EHand::Right) return Snapshot.RightActor;
	return nullptr;
}

static UItemInstance* GetSnapshotInstance(const FCraftingSnapshot& Snapshot, EHand Hand, bool bStation)
{
	if (bStation) return Snapshot.StationInstance;
	if (Hand == EHand::Left) return Snapshot.LeftInstance;
	if (Hand == EHand::Right) return Snapshot.RightInstance;
	return nullptr;
}

static bool SnapshotHasItem(const FCraftingSnapshot& Snapshot, EHand Hand, bool bStation)
{
	if (bStation) return Snapshot.StationInstance != nullptr;
	if (Hand == EHand::Left) return !Snapshot.bLeftUnarmed && Snapshot.LeftDefinition != nullptr;
	if (Hand == EHand::Right) return !Snapshot.bRightUnarmed && Snapshot.RightDefinition != nullptr;
	return false;
}

static bool SlotMatchesDefinition(const FCraftingSlot& Slot, const UItemDefinition* Definition)
{
	if (!Definition) return false;

	if (Slot.Match == ECraftingSlotMatchMode::ExactDefinition)
	{
		return Definition == Slot.ExactDefinition;
	}

	if (Slot.Match == ECraftingSlotMatchMode::RequiredTag)
	{
		return Slot.RequiredTag.IsValid() && Definition->ItemTags.HasTag(Slot.RequiredTag);
	}

	// Sharpenable waits on hone / durability flags.
	return false;
}

static bool TryBindRecipe(const UCraftingRecipeDefinition* Recipe, const FCraftingSnapshot& Snapshot, FCraftingMatch& OutMatch)
{
	if (!Recipe) return false;
	if (Recipe->Slots.Num() == 0) return false;

	const bool bHandsRecipe = Recipe->Station.IsNull();
	const bool bSnapshotHasStation = !Snapshot.Station.IsNull() || Snapshot.StationInstance != nullptr;
	if (bHandsRecipe && bSnapshotHasStation) return false;
	if (!bHandsRecipe && Snapshot.Station != Recipe->Station) return false;

	TArray<bool> UsedHands;
	UsedHands.SetNumZeroed(2);
	bool bUsedStation = false;

	auto BindSnapshotItem = [&](int32 SlotIndex, EHand Hand, bool bStation)
	{
		FCraftingSlotBinding Binding;
		Binding.SlotIndex = SlotIndex;
		Binding.Hand = Hand;
		Binding.bStation = bStation;
		Binding.Instance = GetSnapshotInstance(Snapshot, Hand, bStation);
		Binding.Actor = GetSnapshotActor(Snapshot, Hand, bStation);
		OutMatch.Bindings.Add(Binding);

		if (bStation) bUsedStation = true;
		else if (Hand == EHand::Left) UsedHands[0] = true;
		else if (Hand == EHand::Right) UsedHands[1] = true;
	};

	auto TryHand = [&](int32 SlotIndex, const FCraftingSlot& Slot, EHand Hand) -> bool
	{
		const int32 HandIndex = (Hand == EHand::Left) ? 0 : 1;
		if (UsedHands[HandIndex]) return false;
		if (!SnapshotHasItem(Snapshot, Hand, false)) return false;
		if (!SlotMatchesDefinition(Slot, GetSnapshotDefinition(Snapshot, Hand, false))) return false;
		BindSnapshotItem(SlotIndex, Hand, false);
		return true;
	};

	OutMatch.Recipe = Recipe;
	OutMatch.Bindings.Reset();

	for (int32 SlotIndex = 0; SlotIndex < Recipe->Slots.Num(); ++SlotIndex)
	{
		const FCraftingSlot& Slot = Recipe->Slots[SlotIndex];
		if (!Slot.bMustBeHeld && Slot.Role != ECraftingSlotRole::Tool) continue;

		if (TryHand(SlotIndex, Slot, EHand::Left)) continue;
		if (TryHand(SlotIndex, Slot, EHand::Right)) continue;
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < Recipe->Slots.Num(); ++SlotIndex)
	{
		const FCraftingSlot& Slot = Recipe->Slots[SlotIndex];
		if (Slot.bMustBeHeld || Slot.Role == ECraftingSlotRole::Tool) continue;

		if (TryHand(SlotIndex, Slot, EHand::Left)) continue;
		if (TryHand(SlotIndex, Slot, EHand::Right)) continue;

		if (!bHandsRecipe && !bUsedStation && SnapshotHasItem(Snapshot, EHand::None, true)
			&& SlotMatchesDefinition(Slot, GetSnapshotDefinition(Snapshot, EHand::None, true)))
		{
			BindSnapshotItem(SlotIndex, EHand::None, true);
			continue;
		}

		return false;
	}

	if (bHandsRecipe && bUsedStation) return false;

	if (!Snapshot.bLeftUnarmed && !UsedHands[0]) return false;
	if (!Snapshot.bRightUnarmed && !UsedHands[1]) return false;

	return true;
}

TArray<FCraftingMatch> UCraftingRecipeDefinition::FindMatches(
	const FCraftingSnapshot& Snapshot,
	const TArray<UCraftingRecipeDefinition*>& Recipes)
{
	TArray<FCraftingMatch> Matches;

	const bool bAnyExtended = Snapshot.bLeftExtended || Snapshot.bRightExtended;
	const bool bAnyStacked = Snapshot.bLeftStacked || Snapshot.bRightStacked;
	if (bAnyExtended || bAnyStacked) return Matches;

	for (UCraftingRecipeDefinition* Recipe : Recipes)
	{
		FCraftingMatch Match;
		if (TryBindRecipe(Recipe, Snapshot, Match))
		{
			Matches.Add(Match);
		}
	}
	return Matches;
}

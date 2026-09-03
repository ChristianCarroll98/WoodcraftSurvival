// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Crafting/CraftingMinigameDefinition.h"

UCraftingMinigameDefinition::UCraftingMinigameDefinition()
{
}

FPrimaryAssetId UCraftingMinigameDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CraftingMinigame"), GetFName());
}

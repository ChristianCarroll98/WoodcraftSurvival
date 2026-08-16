// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemDefinition.h"

UItemDefinition::UItemDefinition()
{
	DisplayName = FText::FromString("Item");
}

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	// Format: PrimaryAssetType = "ItemDefinition", PrimaryAssetName = the asset's FName
	return FPrimaryAssetId(TEXT("ItemDefinition"), GetFName());
}

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemDefinition.h"

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	// Format: PrimaryAssetType = "ItemDefinition", PrimaryAssetName = the asset's FName
	return FPrimaryAssetId(TEXT("ItemDefinition"), GetFName());
}

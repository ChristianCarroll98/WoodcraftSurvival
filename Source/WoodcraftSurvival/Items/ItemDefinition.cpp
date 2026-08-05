#include "ItemDefinition.h"

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	// Format: PrimaryAssetType = "ItemDefinition", PrimaryAssetName = the asset's FName
	return FPrimaryAssetId(TEXT("ItemDefinition"), GetFName());
}

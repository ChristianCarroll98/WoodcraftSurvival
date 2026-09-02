// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Fragments/ItemFragment.h"
#include "GameplayTagContainer.h"
#include "ItemDefinition.generated.h"

class UPrimaryDataAsset;
class UMaterialInterface;

/**
 * Static definition of an item (the template).
 * Create one Data Asset per item type (e.g. DA_Stick, DA_Stone).
 * This class never holds runtime state — that lives on UItemInstance.
 */
UCLASS(BlueprintType)
class WOODCRAFTSURVIVAL_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UItemDefinition();

	/** Mesh used when the item is in the world or held in hand */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|Collision")
	TSoftObjectPtr<UStaticMesh> PrimaryMesh;

	/** Optional secondary mesh for tool/weapon heads */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|Collision")
	TSoftObjectPtr<UStaticMesh> SecondaryMesh;

	/** Relative transform of the secondary mesh from the primary mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|Collision")
	FTransform SecondaryRelativeTransform = FTransform::Identity;

	/**
	 * Optional slot-0 override on PrimaryMesh. Empty = mesh asset default.
	 * Use when the same static mesh is shared (e.g. assembled-only ORM).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|Collision")
	TSoftObjectPtr<UMaterialInterface> PrimaryMaterialOverride;

	/**
	 * Optional slot-0 override on SecondaryMesh. Empty = mesh asset default.
	 * StoneHatchet head: same SM_SharpenedStone, MI with assembled ORM.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|Collision", meta = (EditCondition = "SecondaryMesh != nullptr"))
	TSoftObjectPtr<UMaterialInterface> SecondaryMaterialOverride;

	/** Display name for UI */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|UI")
	FText DisplayName;

	/** Icon for UI */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|UI")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Gameplay tags that describe this item (Item.Resource.Wood, Item.Fuel, Item.Tool, …) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer ItemTags;

	/**
	 * Modular capabilities of this item.
	 * Add concrete fragments here (Stackable, Equippable, Durability, Fuel, etc.).
	 * "Instanced" makes the fragments owned by this asset and editable inline in the editor.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Fragments")
	TArray<TObjectPtr<UItemFragment>> Fragments;

	/** Returns the Primary Asset ID used by the Asset Manager (ItemDefinition:AssetName). */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Finds a fragment of the requested type.
	 * Returns nullptr if this definition does not have that fragment.
	 * Example: const UStackableItemFragment* Stack = Definition->FindFragment<UStackableItemFragment>();
	 */
	template<typename T>
	const T* FindFragment() const
	{
		for (UItemFragment* Fragment : Fragments)
		{
			if (const T* Casted = Cast<T>(Fragment))
			{
				return Casted;
			}
		}
		return nullptr;
	}
};

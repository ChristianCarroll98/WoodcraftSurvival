#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Fragments/ItemFragment.h"
#include "ItemDefinition.generated.h"

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

	/** Mesh used when the item is in the world or held in hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** Icon for UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Gameplay tags that describe this item (Item.Resource.Wood, Item.Fuel, Item.Tool, …). */
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
	 * Example: const UStackableFragment* Stack = Definition->FindFragment<UStackableFragment>();
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

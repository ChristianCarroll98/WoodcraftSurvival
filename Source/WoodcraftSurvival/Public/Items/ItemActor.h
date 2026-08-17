// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "ItemActorInterface.h"
#include "ItemActor.generated.h"

class UItemInstance;
class UStaticMeshComponent;

/**
 * World representation of an item.
 * Root component is always a StaticMeshComponent.
 * May have a secondary StaticMeshComponent for tool/weapon heads.
 * Holds a UItemInstance and implements IItemActorInterface.
 */
UCLASS()
class WOODCRAFTSURVIVAL_API AItemActor : public AActor, public IItemActorInterface
{
	GENERATED_BODY()

public:

	/** Constructor */
	AItemActor();


	// ---------- IItemActorInterface ----------

	/** Returns the ItemInstance this actor is currently representing */
	virtual UItemInstance* GetItemInstance() const override;
	virtual void InitializeFromInstance(UItemInstance* Instance) override;


	// ---------- Helpers ----------

	/** Returns the primary mesh component of this item actor */
	UStaticMeshComponent* GetItemPrimaryMesh() const;

	/** Returns the secondary mesh component of this item actor, if any */
	UStaticMeshComponent* GetItemSecondaryMesh() const;

	/** Returns the relative transform of the primary mesh component from the ItemDefinition */
	FTransform GetSecondaryRelativeTransform() const;

protected:

	/** The runtime item data this actor represents. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemInstance> ItemInstance;

	/** Root mesh component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|PrimaryMesh")
	TObjectPtr<UStaticMeshComponent> PrimaryMeshComponent;

	/** Optional tool/weapon head mesh component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|PrimaryMesh")
	TObjectPtr<UStaticMeshComponent> SecondaryMeshComponent;
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "ItemActorInterface.h"
#include "Core/Interfaces/IDamageable.h"
#include "ItemActor.generated.h"

class UItemInstance;
class UStaticMeshComponent;

/**
 * World representation of an item.
 * Root component is always a StaticMeshComponent.
 * May have a secondary StaticMeshComponent for tool/weapon heads.
 * Holds a UItemInstance and implements IItemActorInterface + IDamageable.
 */
UCLASS()
class WOODCRAFTSURVIVAL_API AItemActor : public AActor, public IItemActorInterface, public IDamageable
{
	GENERATED_BODY()

public:

	/** Constructor */
	AItemActor();


	// ---------- IItemActorInterface ----------

	/** Returns the ItemInstance this actor is currently representing */
	virtual UItemInstance* GetItemInstance() const override;
	virtual void InitializeFromInstance(UItemInstance* Instance) override;


	// ---------- IDamageable ----------

	/** Currently a no-op. Durability / breakdown logic will live here later. */
	virtual void ApplyDamage(const FDamageInfo& DamageInfo) override;


	// ---------- Helpers ----------

	/** Returns the primary mesh component of this item actor */
	UStaticMeshComponent* GetItemPrimaryMesh() const;

	/** Returns the secondary mesh component of this item actor, if any */
	UStaticMeshComponent* GetItemSecondaryMesh() const;

	/** Returns the relative transform of the primary mesh component from the ItemDefinition */
	FTransform GetSecondaryRelativeTransform() const;


	// ---------- Held State ----------

	/**
	 * Actor currently holding this item (usually the player character).
	 * Set by UHeldItemsComponent on attach, cleared on detach.
	 * Used to prevent held items from damaging each other and for Instigator.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Item|Held")
	TWeakObjectPtr<AActor> Holder;


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

	/** Minimum relative speed (cm/s) required for a collision to deal damage. Tunable later. */
	UPROPERTY(EditDefaultsOnly, Category = "Item|Damage")
	float MinDamageSpeed = 50.f;


	// ---------- Collision Damage ----------

	/** Bound to Primary and Secondary mesh OnComponentHit. */
	UFUNCTION()
	void OnItemMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	/** Enables hit events and binds OnItemMeshHit for the given mesh. */
	void EnableCollisionDamage(UStaticMeshComponent* Mesh);
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Core/Interfaces/IDamageable.h"
#include "HarvestableActor.generated.h"

class UHarvestableDefinition;
class UHarvestableInstance;
class UStaticMeshComponent;

/**
 * Single base actor class for all harvestable resources in the world
 * (trees, stumps, ore veins, clay deposits, crops, etc.).
 *
 * - Always holds a reference to its UHarvestableDefinition.
 * - Holds a reference to its UHarvestableInstance only after promotion
 *   (or from the start for player-planted crops).
 * - AHarvestableActor is disposable: it may be destroyed and later recreated
 *   from the same Instance (streaming, save/load, density management).
 * - Standing harvestables start kinematic / non-simulating.
 * - Primary mesh is set from the Definition (or Instance→Definition).
 */
UCLASS()
class WOODCRAFTSURVIVAL_API AHarvestableActor : public AActor, public IDamageable
{
	GENERATED_BODY()

public:
	AHarvestableActor();

	/** Pure-Definition path used by world generation / foliage (lazy Instance). */
	void InitializeFromDefinition(UHarvestableDefinition* InDefinition);

	/** Full path once an Instance exists. Sets mesh + calls OnHarvestableSpawned on every fragment. */
	void InitializeFromInstance(UHarvestableInstance* InInstance);

	/** Creates a UHarvestableInstance on first meaningful interaction (first real damage, etc.). */
	void PromoteToInstance();

	/** Convenience accessors */
	UHarvestableDefinition* GetHarvestableDefinition() const { return Definition; }
	UHarvestableInstance* GetHarvestableInstance() const { return Instance; }
	bool HasInstance() const { return Instance != nullptr; }

	UStaticMeshComponent* GetPrimaryMeshComponent() const { return PrimaryMeshComponent; }


	// ---------- Interface Overrides ----------

	/** Applies damage from FDamageInfo struct */
	UFUNCTION(BlueprintCallable, Category = "Damage")
	virtual void ApplyDamage(const FDamageInfo& DamageInfo) override;

protected:
	virtual void BeginPlay() override;

	/** Loads and assigns the PrimaryMesh from the current Definition. */
	void SetupMeshFromDefinition();

	/** Calls OnHarvestableSpawned on every fragment of the current Definition. */
	void NotifyFragmentsSpawned();

	/** Called when CurrentHealth reaches ≤ 0. Yield / stump logic will live here later. */
	virtual void HandleDeath();

	/** Always present. The visual and collision body of the harvestable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PrimaryMeshComponent;

	/** Type data. Always valid after Initialize*. */
	UPROPERTY(BlueprintReadOnly, Category = "Harvestable")
	TObjectPtr<UHarvestableDefinition> Definition;

	/**
	 * Runtime state. Null until PromoteToInstance() or until spawned from an existing Instance.
	 * Player-planted crops receive an Instance immediately.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Harvestable")
	TObjectPtr<UHarvestableInstance> Instance;
};

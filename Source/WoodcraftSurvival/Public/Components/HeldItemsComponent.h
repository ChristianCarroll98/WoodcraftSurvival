// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/WoodcraftTypes.h"
#include "Components/ActorComponent.h"
#include "PhysicsControlComponent.h"
#include "HeldItemsComponent.generated.h"

class AItemActor;
class UItemDefinition;

/**
 * Manages items held in both hands.
 * A single component owns both the left and right hand state.
 * Empty hands always hold the Unarmed item so the system never has a null hand.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class WOODCRAFTSURVIVAL_API UHeldItemsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UHeldItemsComponent();

	// ----- Core API -----

	/** Attempts to pick up the given item into the specified hand. */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	bool TryPickup(AItemActor* Item, EHand Hand);

	/** Drops the item in the specified hand and equips Unarmed in its place. */
	UFUNCTION(BlueprintCallable, Category = "Held Item")
	bool TryDrop(EHand Hand);

	/** Returns the item currently held in the given hand (never null after BeginPlay). */
	UFUNCTION(BlueprintPure, Category = "Held Item")
	AItemActor* GetHeldItem(EHand Hand) const;

	/** Returns true if the given hand is holding something other than Unarmed. */
	UFUNCTION(BlueprintPure, Category = "Held Item")
	bool IsHolding(EHand Hand) const;

	/** Returns true if either hand is currently holding a two-handed item. */
	UFUNCTION(BlueprintPure, Category = "Held Item")
	bool IsTwoHanded() const;

	/** Returns both grip transforms at once. Convenient for driving AnimBP every frame. */
	UFUNCTION(BlueprintCallable, Category = "Held Item", meta = (DisplayName = "Get Grip Transforms"))
	void GetGripTransforms(FTransform& GripTransformLeft, FTransform& GripTransformRight) const;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
			FActorComponentTickFunction* ThisTickFunction) override;

	/** Item currently held in the left hand. */
	UPROPERTY(BlueprintReadOnly, Category = "Held Item")
	TObjectPtr<AItemActor> HeldItemLeft;

	/** Item currently held in the right hand. */
	UPROPERTY(BlueprintReadOnly, Category = "Held Item")
	TObjectPtr<AItemActor> HeldItemRight;

	/** Definition used to spawn the Unarmed item when a hand is empty. */
	UPROPERTY(EditDefaultsOnly, Category = "Held Item")
	TObjectPtr<UItemDefinition> UnarmedDefinition;

	/** Physics Control component used to create and remove controls. Assigned in the Player Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Held Item|Physics")
	TObjectPtr<UPhysicsControlComponent> PhysicsControl;

	/** The skeletal mesh that owns the WeaponBone_L / WeaponBone_R sockets (SK_FPArmsAnimRef). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Held Item|Physics")
	TObjectPtr<USkeletalMeshComponent> AnimRefMesh;

private:

	FName GetWeaponBoneName(EHand Hand) const;

	FName GetHandBoneName(EHand Hand) const;

	/** Equips the Unarmed item into the specified hand. */
	void EquipUnarmed(EHand Hand);

	/** Creates a Physics Control that holds the item in the given hand. */
	void AttachItemToControl(AItemActor* Item, EHand Hand);

	/** Destroys / disables the Physics Control for the given hand. */
	void DetachItemFromControl(EHand Hand);

	/** Returns the relative transform between the specified weapon bone and hand bone from the current animation frame. */
	FTransform GetRelativeTransformBetweenWeaponAndHandBones(EHand Hand) const;
	
	/** Returns the world-space transform for the weapon bone specified for the specified hand. */
	FTransform GetWeaponboneTransform(EHand Hand) const;

	/**
	 * Returns the world-space grip transform for the given hand.
	 * Used by Control Rig IK. Computes from the item's GripPrimary / GripSecondary socket
	 * and applies left-hand mirroring when required.
	 */
	FTransform GetGripTransform(EHand Hand) const;

	/** Updates collision based on the distance an item is from the control parent to prevent items getting stuck. */
	void PreventItemStuck(EHand Hand);

	/** Currently active control name for the left hand. */
	UPROPERTY()
	FName ActiveControlLeft;

	/** Currently active control name for the right hand. */
	UPROPERTY()
	FName ActiveControlRight;

	bool bLeftItemStuck = false;
	bool bRightItemStuck = false;

	bool GetItemStuck(EHand Hand);
	void SetItemStuck(EHand Hand, bool bStuck);
};

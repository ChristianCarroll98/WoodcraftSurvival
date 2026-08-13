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

	// ---------- private methods ----------

	/** Equips the Unarmed item into the specified hand. */
	void EquipUnarmed(EHand Hand);

	/** Creates a Physics Control that holds the item in the given hand. */
	void AttachItemToControl(AItemActor* Item, EHand Hand);

	/** Destroys / disables the Physics Control for the given hand. */
	void DetachItemFromControl(EHand Hand);

	/** Updates collision based on the distance an item is from the control parent to prevent items getting stuck. */
	void PreventItemStuck(EHand Hand);

	/** Sets the stuck status for the item in the given hand. */
	void SetItemStuck(EHand Hand, bool bStuck);


	// ---------- const helpers ----------

	/** Returns the name of the weapon bone for the given hand. */
	FName GetWeaponBoneName(EHand Hand) const;

	/** Returns the name of the hand bone for the given hand. */
	FName GetHandBoneName(EHand Hand) const;

	/** Returns the relative transform between the specified weapon bone and hand bone from the current animation frame. */
	FTransform GetRelativeTransformBetweenWeaponAndHandBones(EHand Hand) const;

	/** Returns the world-space transform for the weapon bone specified for the specified hand. */
	FTransform GetWeaponBoneTransform(EHand Hand) const;

	/** Returns the world-space IK target transform for the given hand. */
	FTransform GetGripTransform(EHand Hand) const;

	/** Returns true if the item in the given hand is currently stuck (too far from the control parent). */
	bool GetItemStuck(EHand Hand) const;
	
	/** Returns the item actor that the player is currently looking at, within the specified max distance. */
	AItemActor* FindLookedAtItem(float Radius = 5.f, float MaxDistance = 250.f) const;

	// Returns all equippable items currently in range (for the gray highlight)
	//void GetItemsInPickupRange(TArray<AItemActor*>& OutItems, float Radius = 120.f) const;

	// ---------- private class variables ----------

	/** Currently active control name for the left hand. */
	FName ActiveControlLeft;

	/** Currently active control name for the right hand. */
	FName ActiveControlRight;

	/** Whether the item in the left hand is currently stuck (too far from the control parent). */
	bool bLeftItemStuck = false;

	/** Whether the item in the right hand is currently stuck (too far from the control parent). */
	bool bRightItemStuck = false;
};

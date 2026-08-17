// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Core/WoodcraftTypes.h"
#include <Components/ActorComponent.h>
#include "HeldItemsComponent.generated.h"

class UPhysicsControlComponent;
class USkeletalMeshComponent;
class AItemActor;
class UItemDefinition;
class UItemFactorySubsystem;
class UFPArmsAnimInstance;

/** Temporary data for an in-progress pickup animation on one hand. */
struct FPendingPickupData
{
	/** The item that is currently being picked up. */
	TWeakObjectPtr<AItemActor> TargetItem;

	/** The neutral animation pose to send to the animation blueprint while the pickup is in progress. */
	TSoftObjectPtr<UAnimSequence> NeutralPose;

	/** The extended animation pose to send to the animation blueprint while the pickup is in progress. */
	TSoftObjectPtr<UAnimSequence> ExtendedPose;
};

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

	// ---------- Editor Configuration ----------

	/** Default pickup montage for left hand */
	UPROPERTY(EditDefaultsOnly, Category = "Configuration|Animation")
	TObjectPtr<UAnimMontage> DefaultPickupMontageLeft;

	/** Default pickup montage for right hand */
	UPROPERTY(EditDefaultsOnly, Category = "Configuration|Animation")
	TObjectPtr<UAnimMontage> DefaultPickupMontageRight;

	/** Default drop montage for left hand */
	//UPROPERTY(EditDefaultsOnly, Category = "Animation|Drop")
	//TObjectPtr<UAnimMontage> DefaultDropMontageLeft;

	/** Default drop montage for right hand */
	//UPROPERTY(EditDefaultsOnly, Category = "Animation|Drop")
	//TObjectPtr<UAnimMontage> DefaultDropMontageRight;

	/** Unarmed Neutral pose */
	UPROPERTY(EditDefaultsOnly, Category = "Configuration|Unarmed")
	TObjectPtr<UAnimSequence> UnarmedNeutralPose;

	/** Unarmed Extended pose */
	UPROPERTY(EditDefaultsOnly, Category = "Configuration|Unarmed")
	TObjectPtr<UAnimSequence> UnarmedExtendedPose;

	/** Definition used to spawn the Unarmed item when a hand is empty. */
	UPROPERTY(EditDefaultsOnly, Category = "Configuration|Unarmed")
	TObjectPtr<UItemDefinition> UnarmedDefinition;


	// ---------- Run-Time Configuration ----------

	/** Physics Control component used to create and remove controls. Assigned in the Player Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|RunTime|Physics")
	TObjectPtr<UPhysicsControlComponent> PhysicsControl;

	/** The skeletal mesh that controls player animations. Assigned in the Player Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|RunTime|Physics")
	TObjectPtr<USkeletalMeshComponent> AnimRefMesh;


	// ---------- Core API ----------

	/** 
	 * Attempts to pick up the item the player is currently looking at,
	 * or drop the item in the given hand if already holding something.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoreAPI")
	bool TryPickupOrDrop(EHand Hand, FString& OutResult);

	/** Returns the item currently held in the given hand (never null after BeginPlay). */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	AItemActor* GetHeldItem(EHand Hand) const;

	/** Returns the primary hand that is holding a two-handed item, or EHand::None if neither is. */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	EHand GetIsHoldingTwoHanded() const;

	/** Returns both grip transforms at once. Convenient for driving AnimBP every frame. */
	UFUNCTION(BlueprintCallable, Category = "CoreAPI", meta = (DisplayName = "Get Grip Transforms"))
	void GetGripTransforms(FTransform& GripTransformLeft, FTransform& GripTransformRight) const;

	/** Completes the pickup animation for the specified hand. */
	UFUNCTION(BlueprintCallable, Category = "CoreAPI")
	void CompletePickup(EHand Hand);


protected:
	
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
			FActorComponentTickFunction* ThisTickFunction) override;
	

private:

	// ---------- private methods ----------

	/** Attempts to pick up the given item into the specified hand. */
	bool TryPickup(AItemActor* Item, EHand Hand, FString& OutResult);

	/** Drops the item in the specified hand and equips Unarmed in its place. */
	bool TryDrop(EHand Hand, FString& OutResult);

	/** Begins the pickup animation for the given item and hand. */
	bool BeginPickupAnimation(AItemActor* Item, EHand Hand, FString& OutResult);

	/** Async loads neutral and extended poses and pushes to AnimInstance when complete */
	void LoadAndPushPoses(EHand Hand);

	/** Spawns an Unarmed item and equips to the specified hand. */
	bool EquipUnarmed(EHand Hand, FString& OutResult);

	/** Creates a Physics Control that holds the item in the given hand. */
	bool AttachItemToControl(AItemActor* Item, EHand Hand, FString& OutResult);

	/** Destroys / disables the Physics Control for the given hand. */
	void DetachItemFromControl(EHand Hand);

	/** Updates collision based on the distance an item is from the control parent to prevent items getting stuck. */
	void PreventItemStuck(EHand Hand);

	/** Returns a reference to the pending pickup data for the given hand. */
	FPendingPickupData& GetPendingPickup(EHand Hand);


	// ---------- Callbacks ----------

	/** Callback for when the soft pointers to the item animations are loaded */
	void OnPosesLoaded(EHand Hand);


	// ---------- Const Helpers ----------

	/** Returns the name of the weapon bone for the given hand. */
	const FName GetWeaponBoneName(EHand Hand) const;

	/** Returns the name of the hand bone for the given hand. */
	const FName GetHandBoneName(EHand Hand) const;

	/** Returns the relative transform between the specified weapon bone and hand bone from the current animation frame. */
	const FTransform GetRelativeTransformBetweenWeaponAndHandBones(EHand Hand) const;

	/** Returns the world-space transform for the weapon bone specified for the specified hand. */
	const FTransform GetWeaponBoneTransform(EHand Hand) const;

	/** Returns the world-space IK target transform for the given hand. */
	const FTransform GetGripTransform(EHand Hand) const;

	/** Returns the item actor that the player is currently looking at, within the specified max distance. */
	AItemActor* FindLookedAtItem(float Radius = 5.f, float MaxDistance = 250.f) const;

	// Returns all equippable items currently in range (for the gray highlight)
	//void GetItemsInPickupRange(TArray<AItemActor*>& OutItems, float Radius = 120.f) const;

	/** Returns true if the item in the given hand is the Unarmed item. */
	const bool GetIsUnarmed(EHand Hand) const;

	/** Plays pickup montage for the specified hand */
	bool PlayPickupMontage(EHand Hand, FString& OutResult) const;

	/** Plays drop montage for the specified hand */
	//bool PlayDropMontage(EHand Hand, FString& OutResult) const;


	// ---------- private class variables ----------

	/** The item factory subsystem used to create and spawn item actors. */
	TObjectPtr<UItemFactorySubsystem> ItemFactory;

	/** The animation instance for the first-person arms mesh. */
	TObjectPtr<UFPArmsAnimInstance> AnimInstance;

	/** Item currently held in the left hand. */
	TObjectPtr<AItemActor> HeldItemLeft;

	/** Item currently held in the right hand. */
	TObjectPtr<AItemActor> HeldItemRight;

	/** Temporary data for an in-progress pickup animation on the left hand. */
	FPendingPickupData PendingPickupLeft;

	/** Temporary data for an in-progress pickup animation on the right hand. */
	FPendingPickupData PendingPickupRight;

	/** Currently active control name for the left hand. */
	FName ActiveControlLeft;

	/** Currently active control name for the right hand. */
	FName ActiveControlRight;

	/** Whether the item in the left hand is currently stuck (too far from the control parent). */
	bool bLeftItemStuck = false;

	/** Whether the item in the right hand is currently stuck (too far from the control parent). */
	bool bRightItemStuck = false;
};

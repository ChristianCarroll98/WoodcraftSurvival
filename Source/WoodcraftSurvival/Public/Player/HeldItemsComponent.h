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
 * Per-hand runtime state owned by UHeldItemsComponent.
 * Keeps Left/Right data in one place and makes adding new per-hand fields (velocity history, etc.) clean.
 */
struct FHandState
{
	/** Item currently held in this hand (never null after BeginPlay — Unarmed fills empty hands). */
	TObjectPtr<AItemActor> HeldItem = nullptr;

	/** Temporary data for an in-progress pickup animation. */
	FPendingPickupData PendingPickup;

	/** Currently active Physics Control name for this hand. */
	FName ActiveControl = NAME_None;

	/** Whether the held item is currently treated as stuck (too far from the control parent). */
	bool bItemStuck = false;

	/** Whether this hand is in the extended (strike-ready) pose. */
	bool bExtended = false;

	/**
	 * Linear velocity of the held item’s primary mesh from the previous tick.
	 * Used by incidence so we measure the pre-bounce swing direction instead of the rebound.
	 */
	FVector LastItemVelocity = FVector::ZeroVector;

	/**
	 * True while procedural swing orientation is actively driving this hand’s control.
	 * Used to edge-trigger skeletal-anim disable/restore and target clear (A+D).
	 */
	bool bProceduralOrientActive = false;
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

	/** Sets whether the given hand is in the extended (ready to strike) state.
	 *  Call from Player BP alongside the anim extend bool. Gates damage and future orientation. */
	UFUNCTION(BlueprintCallable, Category = "CoreAPI")
	void SetExtended(EHand Hand, bool bExtended);

	/** Returns whether the given hand is currently extended. */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	bool GetIsExtended(EHand Hand) const;

	/** Returns which hand is holding the given item, or EHand::None. */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	EHand GetHandHoldingItem(const AItemActor* Item) const;

	/** Returns the active Physics Control name for the hand, or NAME_None. */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	FName GetActiveControlName(EHand Hand) const;

	/** Returns the Body Modifier set name used for the held item in this hand (HeldItemLeft / HeldItemRight). */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	FName GetHeldItemModifierSet(EHand Hand) const;

	/**
	 * Linear velocity of the held item’s primary mesh from the previous tick.
	 * Used by incidence angle checks so we measure pre-bounce swing direction.
	 */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	FVector GetLastItemVelocity(EHand Hand) const;

	/**
	 * Raw look delta this frame (X = yaw / screen horizontal, Y = pitch / screen vertical).
	 * Call from Player BP each tick. Used as screen-space swing intent for procedural orientation.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoreAPI")
	void SetLookDelta(FVector2D RawDelta);


protected:
	
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
			FActorComponentTickFunction* ThisTickFunction) override;
	

private:

	// ---------- private methods ----------

	/** Returns a mutable reference to the per-hand state. */
	FHandState& GetHandState(EHand Hand);

	/** Returns a const reference to the per-hand state. */
	const FHandState& GetHandState(EHand Hand) const;

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

	/**
	 * While extended and above GMinSwingOrientSpeed, drives the Physics Control angular
	 * target so the preferred strike axis aligns with screen-space look intent (SetLookDelta).
	 * Rotation is constrained to WeaponBone Z and rate-limited. Snaps back to skeletal when
	 * relative speed falls under the threshold.
	 */
	void UpdateProceduralOrientation(EHand Hand, float DeltaTime);

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

	/** Returns true if the item in the given hand is the Unarmed item. */
	const bool GetIsUnarmed(EHand Hand) const;

	/** Plays pickup montage for the specified hand */
	bool PlayPickupMontage(EHand Hand, FString& OutResult) const;


	// ---------- private class variables ----------

	/** The item factory subsystem used to create and spawn item actors. */
	TObjectPtr<UItemFactorySubsystem> ItemFactory;

	/** The animation instance for the first-person arms mesh. */
	TObjectPtr<UFPArmsAnimInstance> AnimInstance;

	/** Per-hand runtime state (item, control, extended, velocity history, etc.). */
	FHandState HandLeft;
	FHandState HandRight;

	/**
	 * Raw look delta for this frame (X = horizontal, Y = vertical). Set via SetLookDelta from BP.
	 * Screen-space swing intent for procedural orientation.
	 */
	FVector2D LookDelta = FVector2D::ZeroVector;
};

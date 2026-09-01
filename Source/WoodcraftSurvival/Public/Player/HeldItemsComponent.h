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

	/**
	 * Angular scale at attach:
	 * Clamp(Pow(Mass / MassRef, MassExp) * (1 + Pow(COMLever / LeverRef, LeverExp)), Min, Max).
	 */
	float MassScale = 1.0f;

	/**
	 * Linear scale at attach:
	 * Clamp(Pow(Mass / LinearRef, LinearExp), Min, Max).
	 * Mild — acceleration drive already tracks body mass.
	 */
	float MassScaleLinear = 1.0f;

	/**
	 * Preferred edge side for procedural orientation: +1 = mesh +Y, −1 = mesh −Y.
	 * Persists across frames for hysteresis; reset when orient deactivates or item detaches.
	 */
	int8 OrientEdgeSign = 1;
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


	// ---------- Strength (tune on the Player BP / PIE, no C++ rebuild) ----------

	/** Look-delta rate at which swipe strength reaches max. Tune from the cyan look-speed print. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Look", meta = (ClampMin = "1.0"))
	float StrengthFullLookSpeed = 240.f;

	/** Exponent on the 0–1 look-speed factor before lerping swipe strength. 1 = linear, 2+ = ease-in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Look", meta = (ClampMin = "0.1"))
	float StrengthCurveExp = 3.f;

	/** Linear strength while not extended. Mostly planted, a little give. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Linear", meta = (ClampMin = "0.0"))
	float LinearStrengthNeutral = 5.5f;

	/** Linear strength while extended with no / slow look. Tight in-hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Linear", meta = (ClampMin = "0.0"))
	float LinearStrengthBaseline = 8.0f;

	/** Linear strength at high look speed while extended. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Linear", meta = (ClampMin = "0.0"))
	float LinearStrengthMax = 13.0f;

	/** Linear mass (kg) at which MassScaleLinear == 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Linear", meta = (ClampMin = "0.01"))
	float LinearMassRef = 1.67f;

	/** Exponent on (mass / LinearMassRef). Acceleration drive already tracks body mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Linear", meta = (ClampMin = "0.0"))
	float LinearMassExp = 0.5f;

	/** Floor on MassScaleLinear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Linear", meta = (ClampMin = "0.01"))
	float LinearMassScaleMin = 0.7f;

	/** Safety rail on MassScaleLinear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Linear", meta = (ClampMin = "0.01"))
	float LinearMassScaleMax = 4.0f;

	/** Angular strength while not extended. Softer than linear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.0"))
	float AngularStrengthNeutral = 2.5f;

	/** Angular strength while extended with no / slow look. Slight lag floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.0"))
	float AngularStrengthBaseline = 1.8f;

	/** Angular strength at high look speed while extended. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.0"))
	float AngularStrengthMax = 11.0f;

	/** Angular mass (kg) at which the mass term of MassScale == 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.01"))
	float AngularMassRef = 1.2f;

	/** Exponent on (mass / AngularMassRef). 0.5 keeps mass from dominating lever. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.0"))
	float AngularMassExp = 0.5f;

	/** COM-to-origin distance (cm) at which the angular lever term is +1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "1.0"))
	float AngularLeverRef = 25.f;

	/** Exponent on (lever / AngularLeverRef) added into MassScale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.0"))
	float AngularLeverExp = 1.0f;

	/** Floor on angular MassScale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.01"))
	float AngularMassScaleMin = 0.5f;

	/** Safety rail on angular MassScale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength|Angular", meta = (ClampMin = "0.01"))
	float AngularMassScaleMax = 8.0f;

	/** Held-item gravity scale. 0 = no sag / no COM-couple from gravity. World drop restores full gravity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strength", meta = (ClampMin = "0.0"))
	float GravityMultiplier = 0.f;


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
	 *  Call from Player BP alongside the anim extend bool. Gates damage and procedural orientation. */
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

	/** Look-delta rate this frame (LookDelta.Size() / DeltaTime). Used to gate orient snap and damage. */
	UFUNCTION(BlueprintPure, Category = "CoreAPI")
	float GetLookSpeed() const;


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
	 * Ignore (stuck) or Block (free) world/item/harvestable responses on Primary and Secondary.
	 * Secondary must be included — a dual-mesh head will hold the whole tool in a wall otherwise.
	 */
	void SetHeldItemStuckResponses(AItemActor* Item, bool bStuck);

	/**
	 * While extended and above GMinItemSpeed, drives the Physics Control target so the
	 * preferred strike axis aligns with screen-space look intent (SetLookDelta).
	 * Twist axis is WeaponBone +Z; pivot is the Hand bone (wrist). Strengths live in UpdateControlStrengths.
	 */
	void UpdateProceduralOrientation(EHand Hand, float DeltaTime);

	/**
	 * Neutral / extended-idle / look-speed swipe strengths for this hand.
	 * Runs for every held item (including Unarmed and StrikeMode None). Not gated on edge orient.
	 */
	void UpdateControlStrengths(EHand Hand);

	/**
	 * Applies mass-scaled linear + angular strengths (and the matching damping) to the
	 * active Physics Control for this hand. Multipliers are the Strength category values.
	 */
	void ApplyControlStrengths(EHand Hand, float AngularMultiplier, float LinearMultiplier);

	/**
	 * Pins the Physics Control point to the AnimRef wrist (Hand relative to WeaponBone).
	 * Item-local CP from the hold pose, not the live simulated mesh.
	 */
	void ApplyWristControlPoint(EHand Hand);

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

	/** LookDelta.Size() / DeltaTime, updated each tick. */
	float LookSpeed = 0.f;

	/** Rolling 2s peaks for on-screen debug (look rate + right-hand relative item speed). */
	float DebugPeakLookSpeed = 0.f;
	float DebugPeakLookSpeedTime = 0.f;
	float DebugPeakSwingSpeed = 0.f;
	float DebugPeakSwingSpeedTime = 0.f;
};

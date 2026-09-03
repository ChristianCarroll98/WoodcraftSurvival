// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/HeldItemsComponent.h"
#include "Items/ItemActor.h"
#include "Items/ItemFactorySubsystem.h"
#include "Items/ItemInstance.h"
#include "Items/Fragments/EquippableItemFragment.h"
#include "Core/WoodcraftTypes.h"
#include "Player/FPArmsAnimInstance.h"
#include <PhysicsControlComponent.h>
#include <Engine/AssetManager.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/Pawn.h>
#include <DrawDebugHelpers.h>


// --------------------------------------------
//                 Constructor
// --------------------------------------------
UHeldItemsComponent::UHeldItemsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// --------------------------------------------
//              Public - Core API
// --------------------------------------------
#pragma region CoreAPI

bool UHeldItemsComponent::TryPickupOrDrop(EHand Hand, FString& OutResult)
{
	if (Hand == EHand::None)
	{
		OutResult += TEXT("Hand == None");
		return false;
	}

	// Target locked at input time
	AItemActor* LookedAtItem = FindLookedAtItem();
	const bool bHandIsUnarmed = GetIsUnarmed(Hand);

	if (LookedAtItem)
	{
		if (!bHandIsUnarmed) return false; // Occupied + looking at item -> do nothing (swap later)

		bool bSuccess = BeginPickupAnimation(LookedAtItem, Hand, OutResult);
		if (!bSuccess) OutResult = TEXT("BeginPickupAnimation: ") + OutResult;
		return bSuccess;
	}

	// Looking at empty space
	if (!bHandIsUnarmed)
	{
		// TODO: add drop montage??
		bool bSuccess = TryDrop(Hand, OutResult);
		if (!bSuccess) OutResult = TEXT("TryDrop: ") + OutResult;
		return bSuccess;
	}

	// Empty + Unarmed -> nothing
	return false;
}

AItemActor* UHeldItemsComponent::GetHeldItem(EHand Hand) const
{
	if (Hand == EHand::None) return nullptr;
	return GetHandState(Hand).HeldItem;
}

void UHeldItemsComponent::SetExtended(EHand Hand, bool bExtended)
{
	if (Hand == EHand::None) return;

	FHandState& State = GetHandState(Hand);
	if (State.bExtended == bExtended) return;

	State.bExtended = bExtended;
	OnHeldItemsChanged.Broadcast();
}

bool UHeldItemsComponent::GetIsExtended(EHand Hand) const
{
	if (Hand == EHand::None) return false;
	return GetHandState(Hand).bExtended;
}

EHand UHeldItemsComponent::GetHandHoldingItem(const AItemActor* Item) const
{
	if (!Item) return EHand::None;
	if (HandLeft.HeldItem == Item) return EHand::Left;
	if (HandRight.HeldItem == Item) return EHand::Right;
	return EHand::None;
}

FName UHeldItemsComponent::GetActiveControlName(EHand Hand) const
{
	if (Hand == EHand::None) return NAME_None;
	return GetHandState(Hand).ActiveControl;
}

FName UHeldItemsComponent::GetHeldItemModifierSet(EHand Hand) const
{
	if (Hand == EHand::Left) return TEXT("HeldItemLeft");
	if (Hand == EHand::Right) return TEXT("HeldItemRight");
	return NAME_None;
}

FVector UHeldItemsComponent::GetLastItemVelocity(EHand Hand) const
{
	if (Hand == EHand::None) return FVector::ZeroVector;
	return GetHandState(Hand).LastItemVelocity;
}

void UHeldItemsComponent::SetLookDelta(FVector2D RawDelta)
{
	// Y is negated so screen-up matches intent (raw pitch delta is inverted vs CamUp)
	LookDelta.X = RawDelta.X;
	LookDelta.Y = -RawDelta.Y;
}

float UHeldItemsComponent::GetLookSpeed() const
{
	return LookSpeed;
}

EHand UHeldItemsComponent::GetIsHoldingTwoHanded() const
{
	AItemActor* LeftItem = GetHeldItem(EHand::Left);
	if (LeftItem)
	{
		if (const UItemInstance* LeftInstance = LeftItem->GetItemInstance())
		{
			if (const UEquippableItemFragment* LeftEquippable
				= LeftInstance->FindFragment<UEquippableItemFragment>())
			{
				if (LeftEquippable->bTwoHanded) return EHand::Left;
			}
		}
	}

	AItemActor* RightItem = GetHeldItem(EHand::Right);
	if (RightItem)
	{
		if (const UItemInstance* RightInstance = RightItem->GetItemInstance())
		{
			if (const UEquippableItemFragment* RightEquippable
				= RightInstance->FindFragment<UEquippableItemFragment>())
			{
				if (RightEquippable->bTwoHanded) return EHand::Right;
			}
		}
	}

	return EHand::None;
}

void UHeldItemsComponent::GetGripTransforms(FTransform& OutGripTransformLeft,
		FTransform& OutGripTransformRight) const
{
	OutGripTransformLeft = GetGripTransform(EHand::Left);
	OutGripTransformRight = GetGripTransform(EHand::Right);
}

void UHeldItemsComponent::CompletePickup(EHand Hand)
{
	if (Hand == EHand::None)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			GErrorPrefix + TEXT("CompletePickup: Hand == None"));
		return;
	}

	// Grab the correct pending data
	FPendingPickupData& Pending = GetPendingPickup(Hand);

	AItemActor* ItemToPickup = Pending.TargetItem.Get();
	if (!ItemToPickup)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			GErrorPrefix + TEXT("CompletePickup: ItemToPickup Invalid"));
		return;
	}

	FString OutResult;
	bool bSuccess = TryPickup(ItemToPickup, Hand, OutResult);
	if (!bSuccess && GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, GErrorPrefix + OutResult);

	Pending.TargetItem.Reset();

	return;
}

#pragma endregion


// --------------------------------------------
//            Protected - Lifecycle
// --------------------------------------------
#pragma region Lifecycle

void UHeldItemsComponent::BeginPlay()
{
	Super::BeginPlay();

	ItemFactory = GetWorld()->GetSubsystem<UItemFactorySubsystem>();

	if (!AnimRefMesh)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			GErrorPrefix + TEXT("BeginPlay: AnimRefMesh Invalid"));
		return;
	}
	AnimInstance = Cast<UFPArmsAnimInstance>(AnimRefMesh->GetAnimInstance());

	// Both hands start with Unarmed

	FString OutResultLeft;
	bool bSuccess = EquipUnarmed(EHand::Left, OutResultLeft);
	if (!bSuccess && GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			GErrorPrefix + TEXT("EquipUnarmed: ") + OutResultLeft);

	FString OutResultRight;
	bSuccess = EquipUnarmed(EHand::Right, OutResultRight);
	if (!bSuccess && GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			GErrorPrefix + TEXT("EquipUnarmed: ") + OutResultRight);
}

void UHeldItemsComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Sample held-item velocity *before* collision responses for this frame are fully applied.
	// Used by incidence so we measure pre-bounce swing direction.
	auto SampleVelocity = [](FHandState& State)
	{
		if (!State.HeldItem) return;
		if (UStaticMeshComponent* Mesh = State.HeldItem->GetItemPrimaryMesh())
		{
			State.LastItemVelocity = Mesh->GetPhysicsLinearVelocity();
		}
	};
	SampleVelocity(HandLeft);
	SampleVelocity(HandRight);

	LookSpeed = LookDelta.Size() / FMath::Max(DeltaTime, 0.0001f);

	if (GbDebugPrint && GEngine)
	{
		FVector RelativeVel = HandRight.LastItemVelocity;
		if (const AActor* OwnerActor = GetOwner())
		{
			RelativeVel -= OwnerActor->GetVelocity();
		}

		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		const float SwingSpeed = RelativeVel.Size();
		const float PeakWindow = 2.f;

		if (LookSpeed >= DebugPeakLookSpeed || (Now - DebugPeakLookSpeedTime) > PeakWindow)
		{
			DebugPeakLookSpeed = LookSpeed;
			DebugPeakLookSpeedTime = Now;
		}
		if (SwingSpeed >= DebugPeakSwingSpeed || (Now - DebugPeakSwingSpeedTime) > PeakWindow)
		{
			DebugPeakSwingSpeed = SwingSpeed;
			DebugPeakSwingSpeedTime = Now;
		}

		GEngine->AddOnScreenDebugMessage(101, 1.f, FColor::Magenta,
			FString::Printf(TEXT("R swing speed: %.1f  max2s: %.1f"), SwingSpeed, DebugPeakSwingSpeed));

		float Mass = 0.f;
		if (HandRight.HeldItem)
		{
			if (UStaticMeshComponent* Primary = HandRight.HeldItem->GetItemPrimaryMesh())
			{
				Mass = Primary->GetMass();
			}
			if (UStaticMeshComponent* Secondary = HandRight.HeldItem->GetItemSecondaryMesh())
			{
				if (!Secondary->IsWelded()) Mass += Secondary->GetMass();
			}
		}
		GEngine->AddOnScreenDebugMessage(102, 1.f, FColor::Orange,
			FString::Printf(TEXT("R item mass: %.2f"), Mass));

		GEngine->AddOnScreenDebugMessage(103, 1.f, FColor::Cyan,
			FString::Printf(TEXT("Look speed: %.2f  max2s: %.2f"), LookSpeed, DebugPeakLookSpeed));
	}

	PreventItemStuck(EHand::Left);
	PreventItemStuck(EHand::Right);

	UpdateControlStrengths(EHand::Left);
	UpdateControlStrengths(EHand::Right);

	ApplyWristControlPoint(EHand::Left);
	ApplyWristControlPoint(EHand::Right);

	UpdateProceduralOrientation(EHand::Left, DeltaTime);
	UpdateProceduralOrientation(EHand::Right, DeltaTime);
}

FHandState& UHeldItemsComponent::GetHandState(EHand Hand)
{
	return (Hand == EHand::Left) ? HandLeft : HandRight;
}

const FHandState& UHeldItemsComponent::GetHandState(EHand Hand) const
{
	return (Hand == EHand::Left) ? HandLeft : HandRight;
}

#pragma endregion


// --------------------------------------------
//          Private - Pickup and Drop
// --------------------------------------------
#pragma region PickupAndDrop

bool UHeldItemsComponent::TryPickup(AItemActor* Item, EHand Hand, FString& OutResult)
{
	if (!Item)
	{
		OutResult += TEXT("Item invalid");
		return false;
	}
	else if (Hand == EHand::None)
	{
		OutResult += TEXT("Hand == None");
		return false;
	}

	// TODO: Two handed item handling

	// Get current held item
	AItemActor* PreviousItem = GetHeldItem(Hand);

	// If player has Unarmed item, destroy it before attaching new item
	if (PreviousItem && GetIsUnarmed(Hand)) PreviousItem->Destroy();

	bool bSuccess = AttachItemToControl(Item, Hand, OutResult);
	if (!bSuccess) OutResult = TEXT("AttachItemToControl: ") + OutResult;
	return bSuccess;
}

bool UHeldItemsComponent::TryDrop(EHand Hand, FString& OutResult)
{
	AItemActor* Item = GetHeldItem(Hand);

	if (!Item)
	{
		OutResult += TEXT("Could not get held item for hand: ") + UEnum::GetValueAsString(Hand);
		return false;
	}
	else if (Hand == EHand::None)
	{
		OutResult += TEXT("Hand == None");
		return false;
	}

	// Never drop Unarmed
	if (GetIsUnarmed(Hand)) return false;

	DetachItemFromControl(Hand);

	bool bSuccess = EquipUnarmed(Hand, OutResult);
	if (!bSuccess) OutResult = TEXT("EquipUnarmed: ") + OutResult;

	return bSuccess;
}

bool UHeldItemsComponent::BeginPickupAnimation(AItemActor* Item, EHand Hand, FString& OutResult)
{
	if (!Item)
	{
		OutResult += TEXT("Item Invalid");
		return false;
	}
	else if (Hand == EHand::None)
	{
		OutResult += TEXT("Hand == None");
		return false;
	}

	const UItemInstance* Instance = Item->GetItemInstance();
	if (!Instance)
	{
		OutResult += TEXT("ItemInstance invalid");
		return false;
	}

	const UEquippableItemFragment* EquipFrag = Instance->FindFragment<UEquippableItemFragment>();
	if (!EquipFrag)
	{
		OutResult += TEXT("Could not find EquippableItemFragment for Item: " + Item->GetName());
		return false;
	}

	// Store pending data first
	FPendingPickupData& Pending = GetPendingPickup(Hand);
	Pending.TargetItem = MakeWeakObjectPtr(Item);
	Pending.NeutralPose = EquipFrag->NeutralPose;
	Pending.ExtendedPose = EquipFrag->ExtendedPose;

	bool bSuccess = PlayPickupMontage(Hand, OutResult);
	if (!bSuccess) OutResult = TEXT("PlayPickupMontage: ") + OutResult;

	if (bSuccess) LoadAndPushPoses(Hand);

	return bSuccess;
}

bool UHeldItemsComponent::EquipUnarmed(EHand Hand, FString& OutResult)
{
	if (!UnarmedDefinition)
	{
		OutResult += TEXT("UnarmedDefinition invalid");
		return false;
	}
	else if (!ItemFactory)
	{
		OutResult += TEXT("ItemFactory invalid");
		return false;
	}
	else if (Hand == EHand::None)
	{
		OutResult += TEXT("Hand == None");
		return false;
	}

	// Create new UnarmedActor from definition at current hand transform
	AItemActor* UnarmedActor = ItemFactory->SpawnItemActorFromDefinition(
		UnarmedDefinition,
		GetWeaponBoneTransform(Hand)
	);
	if (!UnarmedActor)
	{
		OutResult += TEXT("UnarmedActor invalid");
		return false;
	}

	// Visually hide unarmed actor
	UnarmedActor->SetActorHiddenInGame(true);

	bool bSuccess = AttachItemToControl(UnarmedActor, Hand, OutResult);
	if (!bSuccess)
	{
		OutResult = TEXT("AttachItemToControl: ") + OutResult;
		return false;
	}

	if (!AnimInstance)
	{
		OutResult += TEXT("AnimInstance invalid");
		return false;
	}

	AnimInstance->SetHoldPose(Hand, UnarmedNeutralPose, UnarmedExtendedPose);
	return true;
}

#pragma endregion


// --------------------------------------------
//             Private - Animation
// --------------------------------------------
#pragma region Animation

void UHeldItemsComponent::LoadAndPushPoses(EHand Hand)
{
	FPendingPickupData& Pending = GetPendingPickup(Hand);

	TArray<FSoftObjectPath> PathsToLoad;
	if (Pending.NeutralPose.IsPending())  PathsToLoad.Add(Pending.NeutralPose.ToSoftObjectPath());
	if (Pending.ExtendedPose.IsPending()) PathsToLoad.Add(Pending.ExtendedPose.ToSoftObjectPath());

	if (PathsToLoad.Num() == 0)
	{
		// Already loaded or null
		OnPosesLoaded(Hand);
		return;
	}

	// Load Async
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.RequestAsyncLoad(
		PathsToLoad,
		FStreamableDelegate::CreateUObject(this, &UHeldItemsComponent::OnPosesLoaded, Hand)
	);
}

void UHeldItemsComponent::OnPosesLoaded(EHand Hand)
{
	FPendingPickupData& Pending = GetPendingPickup(Hand);

	UAnimSequence* Neutral = Pending.NeutralPose.Get();
	UAnimSequence* Extended = Pending.ExtendedPose.Get();

	if (!AnimInstance)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			TEXT("OnPosesLoaded: AnimInstance invalid"));
		return;
	}

	AnimInstance->SetHoldPose(Hand, Neutral, Extended);

	Pending.NeutralPose.Reset();
	Pending.ExtendedPose.Reset();
}

bool UHeldItemsComponent::PlayPickupMontage(EHand Hand, FString& OutResult) const
{
	if (!AnimInstance)
	{
		OutResult += TEXT("AnimInstance Invalid");
		return false;
	}
	else if (Hand == EHand::None)
	{
		OutResult += TEXT("Hand == None");
		return false;
	}

	TObjectPtr<UAnimMontage> PickupMontageToPlay = (Hand == EHand::Left) ?
		DefaultPickupMontageLeft : DefaultPickupMontageRight;

	if (!PickupMontageToPlay)
	{
		OutResult += TEXT("PickupMontageToPlay Invalid");
		return false;
	}

	AnimInstance->Montage_Play(PickupMontageToPlay);
	return true;
}

//bool UHeldItemsComponent::PlayDropMontage(EHand Hand, FString& OutResult) const
//{
//	if (!AnimInstance)
//	{
//		OutResult += TEXT("AnimInstance Invalid");
//		return false;
//	}
//	else if (Hand == EHand::None)
//	{
//		OutResult += TEXT("Hand == None");
//		return false;
//	}
//
//	TObjectPtr<UAnimMontage> DropMontageToPlay = (Hand == EHand::Left) ?
//		DefaultDropMontageLeft : DefaultDropMontageRight;
//
//	if (!DropMontageToPlay)
//	{
//		OutResult += TEXT("DropMontageToPlay Invalid");
//		return false;
//	}
//
//	AnimInstance->Montage_Play(DropMontageToPlay);
//	return true;
//}

#pragma endregion


// --------------------------------------------
// Private - Physics Control / Stuck Prevention
// --------------------------------------------
#pragma region PhysicsControl

bool UHeldItemsComponent::AttachItemToControl(AItemActor* Item, EHand Hand, FString& OutResult)
{
	if (!Item)
	{
		OutResult += TEXT("Item Invalid");
		return false;
	}
	else if (Hand == EHand::None)
	{
		OutResult += TEXT("Hand == None");
		return false;
	}
	else if (!PhysicsControl)
	{
		OutResult += TEXT("PhysicsControl Invalid");
		return false;
	}
	else if (!AnimRefMesh)
	{
		OutResult += TEXT("AnimRefMesh Invalid");
		return false;
	}

	UStaticMeshComponent* ItemMesh = Item->GetItemPrimaryMesh();
	if (!ItemMesh)
	{
		OutResult += TEXT("ItemMesh Invalid");
		return false;
	}

	// Destroy any existing control for this hand first
	DetachItemFromControl(Hand);

	if (Hand == EHand::Left)
	{
		ItemMesh->SetRelativeScale3D(FVector(-1.0f, 1.0f, 1.0f)); // Mirror the item mesh for the left hand
	}

	// Disable collision for the item mesh while it's being held
	ItemMesh->SetCollisionResponseToChannel(COLLISION_PLAYER, ECollisionResponse::ECR_Ignore);

	// Mark this item as held by the character (prevents held-item vs held-item damage)
	Item->Holder = GetOwner();
	Item->SetOwner(GetOwner());

	const FName WeaponBoneName = GetWeaponBoneName(Hand);

	// Welded Secondary lives on Primary's BodyInstance. Only add Secondary when it is its own body.
	float EffectiveMass = ItemMesh->GetMass();
	if (UStaticMeshComponent* Secondary = Item->GetItemSecondaryMesh())
	{
		if (!Secondary->IsWelded()) EffectiveMass += Secondary->GetMass();
	}

	// Linear: mild mass term. Acceleration drive already scales force with body mass.
	const float MassScaleLinear = FMath::Clamp(
		FMath::Pow(FMath::Max(EffectiveMass / LinearMassRef, 0.f), LinearMassExp) * LinearMassScaleMul,
		LinearMassScaleMin,
		LinearMassScaleMax);

	// Angular: mass term × COM lever (head offset from item origin). Caps are safety rails.
	const float AngularMassTerm = FMath::Pow(
		FMath::Max(EffectiveMass / AngularMassRef, 0.f), AngularMassExp);
	const float LeverCm = FVector::Dist(ItemMesh->GetCenterOfMass(), ItemMesh->GetComponentLocation());
	const float AngularLeverTerm = 1.0f + FMath::Pow(
		LeverCm / FMath::Max(AngularLeverRef, 1.f), AngularLeverExp);
	const float MassScale = FMath::Clamp(
		AngularMassTerm * AngularLeverTerm * AngularMassScaleMul,
		AngularMassScaleMin,
		AngularMassScaleMax);

	FPhysicsControlData ControlData;
	ControlData.LinearStrength = LinearStrengthNeutral * MassScaleLinear;
	ControlData.LinearDampingRatio = FMath::Max(0.f,
		LinearDampingRatio + LinearDampingMassSlope * (MassScaleLinear - 1.0f));
	ControlData.AngularStrength = AngularStrengthNeutral * MassScale;
	ControlData.AngularDampingRatio = FMath::Max(0.f,
		AngularDampingRatio + AngularDampingMassSlope * (MassScale - 1.0f));
	ControlData.bUseSkeletalAnimation = true;
	ControlData.bDisableCollision = true;
	ControlData.bUseCustomControlPoint = true;
	ControlData.CustomControlPoint = GetRelativeTransformBetweenWeaponAndHandBones(Hand).GetLocation();

	if (GbDebugPrint && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("PhysControl mass=%.2f lever=%.1f angS=%.2f linS=%.2f  L=%.1f A=%.1f"),
				EffectiveMass, LeverCm, MassScale, MassScaleLinear,
				ControlData.LinearStrength, ControlData.AngularStrength));
	}

	FPhysicsControlTarget ControlTarget;
	ControlTarget.bApplyControlPointToTarget = true;

	// Create the control using the AnimRef mesh + correct weapon bone
	FName NewControlName = PhysicsControl->CreateControl(
		AnimRefMesh,
		WeaponBoneName,
		ItemMesh,
		NAME_None,
		ControlData,
		ControlTarget,
		TEXT("HeldItems"),
		"WSPC_"
	);

	// Reduced gravity via Physics Control Body Modifier (one set per hand).
	const FName ModifierSet = GetHeldItemModifierSet(Hand);
	PhysicsControl->DestroyBodyModifiersInSet(ModifierSet);

	FPhysicsControlModifierData ModData;
	ModData.MovementType = EPhysicsMovementType::Simulated;
	ModData.GravityMultiplier = GravityMultiplier;

	PhysicsControl->CreateBodyModifier(ItemMesh, NAME_None, ModifierSet, ModData);
	if (UStaticMeshComponent* Secondary = Item->GetItemSecondaryMesh())
	{
		if (!Secondary->IsWelded())
		{
			PhysicsControl->CreateBodyModifier(Secondary, NAME_None, ModifierSet, ModData);
		}
	}

	FHandState& State = GetHandState(Hand);
	State.HeldItem = Item;
	State.ActiveControl = NewControlName;
	State.MassScale = MassScale;
	State.MassScaleLinear = MassScaleLinear;
	State.LastItemVelocity = FVector::ZeroVector; // reset until next tick samples
	OnHeldItemsChanged.Broadcast();
	return true;
}

void UHeldItemsComponent::DetachItemFromControl(EHand Hand)
{
	if (Hand == EHand::None || !PhysicsControl) return;

	FHandState& State = GetHandState(Hand);

	if (!State.ActiveControl.IsNone())
	{
		PhysicsControl->DestroyControl(State.ActiveControl);
		State.ActiveControl = NAME_None;
	}

	// Remove reduced-gravity body modifiers for this hand
	PhysicsControl->DestroyBodyModifiersInSet(GetHeldItemModifierSet(Hand));

	// Restore collision and scale for the item mesh
	AItemActor* Item = State.HeldItem;

	if (Item)
	{
		// Clear held state
		Item->Holder = nullptr;

		UStaticMeshComponent* ItemMesh = Item->GetItemPrimaryMesh();
		if (ItemMesh)
		{
			if (Hand == EHand::Left)
			{
				ItemMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f)); // Reset scale to normal in case was mirrored for left hand
			}
			ItemMesh->SetCollisionResponseToChannel(COLLISION_PLAYER, ECollisionResponse::ECR_Block);
		}
		SetHeldItemStuckResponses(Item, false);
	}

	State.LastItemVelocity = FVector::ZeroVector;
	State.bProceduralOrientActive = false;
	State.OrientEdgeSign = 1;
	State.bItemStuck = false;
}

void UHeldItemsComponent::SetHeldItemStuckResponses(AItemActor* Item, bool bStuck)
{
	if (!Item) return;

	const ECollisionResponse Response = bStuck ? ECR_Ignore : ECR_Block;

	auto Apply = [Response](UStaticMeshComponent* Mesh)
	{
		if (!Mesh) return;
		Mesh->SetCollisionResponseToChannel(COLLISION_ITEM, Response);
		Mesh->SetCollisionResponseToChannel(COLLISION_HARVESTABLE, Response);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, Response);
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, Response);
		Mesh->WakeRigidBody();
	};

	Apply(Item->GetItemPrimaryMesh());
	Apply(Item->GetItemSecondaryMesh());
}

void UHeldItemsComponent::PreventItemStuck(EHand Hand)
{
	if (Hand == EHand::None || !PhysicsControl || !AnimRefMesh) return;

	AItemActor* Item = GetHeldItem(Hand);
	if (!Item) return;

	UStaticMeshComponent* Mesh = Item->GetItemPrimaryMesh();
	if (!Mesh) return;

	// Check distance between AnimRef hand bone and grip IK target
	const FVector HandBoneLocation = AnimRefMesh->GetSocketLocation(GetHandBoneName(Hand));
	// Get the grip location from the current cached transform for this hand for consistency with anim BP
	const FVector GripLocation = GetGripTransform(Hand).GetLocation();

	const float Distance = FVector::Dist(HandBoneLocation, GripLocation);

	// Hysteresis to prevent flickering - TODO: tune later
	const float EnterUnsafeDistance = 70.0f;
	const float ExitUnsafeDistance = 40.0f;

	bool& bItemStuck = GetHandState(Hand).bItemStuck;

	if (Distance > EnterUnsafeDistance && !bItemStuck)
	{
		bItemStuck = true;
		SetHeldItemStuckResponses(Item, true);
	}
	else if (Distance < ExitUnsafeDistance && bItemStuck)
	{
		bItemStuck = false;
		SetHeldItemStuckResponses(Item, false);
	}
}

void UHeldItemsComponent::ApplyControlStrengths(EHand Hand, float AngularMultiplier, float LinearMultiplier)
{
	if (Hand == EHand::None || !PhysicsControl) return;

	const FHandState& State = GetHandState(Hand);
	if (State.ActiveControl.IsNone()) return;

	const float DampingAngular = FMath::Max(0.f,
		AngularDampingRatio + AngularDampingMassSlope * (State.MassScale - 1.0f));
	const float DampingLinear = FMath::Max(0.f,
		LinearDampingRatio + LinearDampingMassSlope * (State.MassScaleLinear - 1.0f));

	PhysicsControl->SetControlAngularData(
		State.ActiveControl,
		AngularMultiplier * State.MassScale,
		DampingAngular,
		0.f,
		0.f,
		true,
		true,
		false);
	PhysicsControl->SetControlLinearData(
		State.ActiveControl,
		LinearMultiplier * State.MassScaleLinear,
		DampingLinear,
		0.f,
		0.f,
		true,
		true,
		false);
}

void UHeldItemsComponent::ApplyWristControlPoint(EHand Hand)
{
	if (Hand == EHand::None || !PhysicsControl) return;
	const FHandState& State = GetHandState(Hand);
	if (State.ActiveControl.IsNone()) return;

	PhysicsControl->SetControlPoint(
		State.ActiveControl,
		GetRelativeTransformBetweenWeaponAndHandBones(Hand).GetLocation());
}

void UHeldItemsComponent::UpdateControlStrengths(EHand Hand)
{
	if (Hand == EHand::None || !PhysicsControl) return;

	const FHandState& State = GetHandState(Hand);
	if (State.ActiveControl.IsNone() || !State.HeldItem) return;

	const float AngLo = State.bExtended
		? AngularStrengthBaseline
		: AngularStrengthNeutral;
	const float LinLo = State.bExtended
		? LinearStrengthBaseline
		: LinearStrengthNeutral;

	float AngMul = AngLo;
	float LinMul = LinLo;

	if (LookSpeed >= GMinLookSpeed)
	{
		const float LinearT = FMath::Clamp(
			LookSpeed / FMath::Max(StrengthFullLookSpeed, 1.f), 0.f, 1.f);
		const float T = FMath::Pow(LinearT, StrengthCurveExp);
		AngMul = FMath::Lerp(AngLo, AngularStrengthMax, T);
		LinMul = FMath::Lerp(LinLo, LinearStrengthMax, T);
	}

	ApplyControlStrengths(Hand, AngMul, LinMul);

	if (GbDebugPrint && GEngine && Hand == EHand::Right)
	{
		const float DampA = FMath::Max(0.f,
			AngularDampingRatio + AngularDampingMassSlope * (State.MassScale - 1.0f));
		const float DampL = FMath::Max(0.f,
			LinearDampingRatio + LinearDampingMassSlope * (State.MassScaleLinear - 1.0f));
		const float AppliedA = AngMul * State.MassScale;
		const float AppliedL = LinMul * State.MassScaleLinear;
		GEngine->AddOnScreenDebugMessage(104, 1.f, FColor::Green,
			FString::Printf(TEXT("R ctrl %s%s  lookT=%.2f  mul L=%.2f A=%.2f  scale L=%.2f A=%.2f"),
				State.bExtended ? TEXT("EXT") : TEXT("NEU"),
				State.bProceduralOrientActive ? TEXT(" ORIENT") : TEXT(""),
				FMath::Clamp(LookSpeed / FMath::Max(StrengthFullLookSpeed, 1.f), 0.f, 1.f),
				LinMul, AngMul,
				State.MassScaleLinear, State.MassScale));
		GEngine->AddOnScreenDebugMessage(105, 1.f, FColor::Green,
			FString::Printf(TEXT("R applied  L=%.2f A=%.2f  damp L=%.2f A=%.2f"),
				AppliedL, AppliedA, DampL, DampA));
	}
}

void UHeldItemsComponent::UpdateProceduralOrientation(EHand Hand, float DeltaTime)
{
	if (Hand == EHand::None || !PhysicsControl) return;

	FHandState& State = GetHandState(Hand);
	if (State.ActiveControl.IsNone() || !State.HeldItem) return;
	if (GetIsUnarmed(Hand)) return;

	// Edged tools only — Pierce / None / Blunt skip procedural orient (damage still from primitives).
	const UItemInstance* Instance = State.HeldItem->GetItemInstance();
	const UEquippableItemFragment* EquipFrag =
		Instance ? Instance->FindFragment<UEquippableItemFragment>() : nullptr;
	const bool bEdged = EquipFrag
		&& (EquipFrag->StrikeMode == EItemStrikeMode::SingleEdged
			|| EquipFrag->StrikeMode == EItemStrikeMode::DoubleEdged);

	// Relative velocity gates on/off. Orientation drive uses look delta.
	FVector RelativeVel = State.LastItemVelocity;
	if (const AActor* OwnerActor = GetOwner())
	{
		RelativeVel -= OwnerActor->GetVelocity();
	}

	const bool bShouldOrient = bEdged
		&& State.bExtended
		&& RelativeVel.Size() >= GMinItemSpeed;

	// D: only clear target + restore skeletal on the transition off
	if (!bShouldOrient)
	{
		if (State.bProceduralOrientActive)
		{
			State.bProceduralOrientActive = false;
			State.OrientEdgeSign = 1;
			PhysicsControl->SetControlUseSkeletalAnimation(State.ActiveControl, true, 1.f);
			PhysicsControl->SetControlTargetPositionAndOrientation(
				State.ActiveControl,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				0.f,
				true,
				true,
				true,
				false);
		}
		return;
	}

	// A: disable skeletal contribution while procedural is active
	if (!State.bProceduralOrientActive)
	{
		State.bProceduralOrientActive = true;
		PhysicsControl->SetControlUseSkeletalAnimation(State.ActiveControl, false, 0.f);
	}

	// No look this frame → leave the last target so the item keeps momentum instead of hard-stopping.
	if (LookSpeed < GMinLookSpeed)
	{
		return;
	}

	UStaticMeshComponent* Mesh = State.HeldItem->GetItemPrimaryMesh();
	if (!Mesh) return;

	// Camera basis from owning pawn's control rotation (screen right / up)
	FVector CamRight = FVector::RightVector;
	FVector CamUp = FVector::UpVector;
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			FVector CamForward;
			FRotationMatrix(PC->GetControlRotation()).GetScaledAxes(CamForward, CamRight, CamUp);
		}
	}

	const FVector Intent = (CamRight * LookDelta.X + CamUp * LookDelta.Y).GetSafeNormal();
	if (Intent.IsNearlyZero()) return;

	const FTransform MeshXform = Mesh->GetComponentTransform();
	const FTransform BoneXform = GetWeaponBoneTransform(Hand);
	const FVector BoneWorldZ = BoneXform.GetUnitAxis(EAxis::Z);

	// Neutral preferred for mesh +Y = projected WeaponBone +Y (base rotation around Z).
	FVector NeutralY = BoneXform.GetUnitAxis(EAxis::Y);
	NeutralY = NeutralY - FVector::DotProduct(NeutralY, BoneWorldZ) * BoneWorldZ;
	if (NeutralY.SizeSquared() < KINDA_SMALL_NUMBER) return;
	NeutralY.Normalize();

	// Project screen intent into the twist plane.
	FVector ProjIntent = Intent - FVector::DotProduct(Intent, BoneWorldZ) * BoneWorldZ;
	if (ProjIntent.SizeSquared() < KINDA_SMALL_NUMBER) return;
	ProjIntent.Normalize();

	// Signed angle (degrees) from From → To around BoneWorldZ (positive = right-hand rule).
	// Playtest: positive maps to CCW, negative maps to CW from the player's view.
	auto SignedAngleDeg = [&](const FVector& From, const FVector& To) -> float
	{
		const float Dot = FVector::DotProduct(From, To);
		const float CrossZ = FVector::DotProduct(FVector::CrossProduct(From, To), BoneWorldZ);
		return FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	};

	// Choose preferred edge (±Y) with wrist limits + hysteresis.
	// Target is never allowed outside the asymmetric legal band.
	// Right hand (after playtest): negative = CW, positive = CCW.
	// Left hand is mirrored → opposite mapping so the feel matches.
	const float LimitPos = (Hand == EHand::Right) ? GWristLimitCCWDeg : GWristLimitCWDeg;
	const float LimitNeg = (Hand == EHand::Right) ? GWristLimitCWDeg  : GWristLimitCCWDeg;

	const bool bSingle = (EquipFrag->StrikeMode == EItemStrikeMode::SingleEdged);
	const int8 PrevSign = State.OrientEdgeSign;

	const float TwistPlus  = SignedAngleDeg( NeutralY, ProjIntent);
	const float TwistMinus = SignedAngleDeg(-NeutralY, ProjIntent);

	auto IsLegal = [&](float TwistDeg)
	{
		return TwistDeg >= -LimitNeg && TwistDeg <= LimitPos;
	};
	auto IsLegalWithHysteresis = [&](float TwistDeg)
	{
		return TwistDeg >= -(LimitNeg + GWristHysteresisDeg)
			&& TwistDeg <= (LimitPos + GWristHysteresisDeg);
	};

	const bool bPlusLegal = IsLegal(TwistPlus);
	const bool bMinusLegal = IsLegal(TwistMinus);

	int8 BestSign = PrevSign;
	if (bSingle)
	{
		// Stay on +Y until past the limit by hysteresis; return to +Y as soon as it is strictly legal.
		if (PrevSign == 1)
		{
			BestSign = IsLegalWithHysteresis(TwistPlus) ? 1 : -1;
		}
		else
		{
			BestSign = bPlusLegal ? 1 : -1;
		}
	}
	else if (PrevSign == 1 && (IsLegalWithHysteresis(TwistPlus) || !bMinusLegal))
	{
		BestSign = 1;
	}
	else if (PrevSign == -1 && (IsLegalWithHysteresis(TwistMinus) || !bPlusLegal))
	{
		BestSign = -1;
	}
	else
	{
		BestSign = (FMath::Abs(TwistMinus) < FMath::Abs(TwistPlus)) ? -1 : 1;
	}

	State.OrientEdgeSign = BestSign;

	// Scalar twist relative to the chosen edge's neutral — always stays inside the legal band.
	const FVector ChosenNeutral = NeutralY * float(BestSign);
	float DesiredTwistDeg = SignedAngleDeg(ChosenNeutral, ProjIntent);
	DesiredTwistDeg = FMath::Clamp(DesiredTwistDeg, -LimitNeg, LimitPos);

	// Current twist from the live mesh preferred axis (projected).
	const FVector CurrentWorldAxis =
		MeshXform.TransformVectorNoScale(FVector::YAxisVector * float(BestSign)).GetSafeNormal();
	FVector ProjAxis = CurrentWorldAxis - FVector::DotProduct(CurrentWorldAxis, BoneWorldZ) * BoneWorldZ;
	float CurrentTwistDeg = 0.f;
	if (ProjAxis.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		ProjAxis.Normalize();
		CurrentTwistDeg = SignedAngleDeg(ChosenNeutral, ProjAxis);
	}
	// If the mesh is still outside the legal band (lagging through a flip), pull the target to the edge.
	CurrentTwistDeg = FMath::Clamp(CurrentTwistDeg, -LimitNeg, LimitPos);

	// 1D rate-limit within the band only — no absolute-plane path that can cross the forbidden zone.
	const float MaxDegPerSec = 3600.f;
	const float MaxStepDeg = MaxDegPerSec * DeltaTime;
	const float StepTwistDeg = FMath::Clamp(DesiredTwistDeg - CurrentTwistDeg, -MaxStepDeg, MaxStepDeg);
	const float FinalTwistDeg = CurrentTwistDeg + StepTwistDeg;

	const FVector SteppedPreferred =
		FQuat(BoneWorldZ, FMath::DegreesToRadians(FinalTwistDeg)).RotateVector(ChosenNeutral).GetSafeNormal();

	// MakeFromYZ aligns mesh +Y; if we are driving with −Y, feed the opposite so mesh −Y lands on SteppedPreferred.
	FVector OrientY = SteppedPreferred;
	if (BestSign < 0)
	{
		OrientY = -SteppedPreferred;
	}

	const FQuat DesiredWorldRot = FRotationMatrix::MakeFromYZ(OrientY, BoneWorldZ).ToQuat();
	const FQuat ParentWorldRot = BoneXform.GetRotation();
	const FQuat RelativeQuat = ParentWorldRot.Inverse() * DesiredWorldRot;
	const FRotator RelativeRot = RelativeQuat.Rotator();

	// Pivot at the Hand bone (wrist). RelLoc is wrist in WeaponBone / item space.
	// TargetPos orbits the item origin around that point so the wrist stays put.
	const FVector RelLoc = GetRelativeTransformBetweenWeaponAndHandBones(Hand).GetLocation();
	const FVector TargetPos = RelLoc - RelativeQuat.RotateVector(RelLoc);

	PhysicsControl->SetControlTargetPositionAndOrientation(
		State.ActiveControl,
		TargetPos,
		RelativeRot,
		0.f,
		true,
		true,
		true,
		false);

	// Debug: RGB at grip/control point — blue (Z) should stay locked to WeaponBone Z
	// + wrist-limit arrows: Neutral (green), CW limit (cyan), CCW limit (yellow)
	if (GbDebugDraw)
	{
		if (UWorld* World = GetWorld())
		{
			const FVector ControlPointLocal =
				GetRelativeTransformBetweenWeaponAndHandBones(Hand).GetLocation();
			const FVector ControlPointWorld = BoneXform.TransformPosition(ControlPointLocal);
			DrawDebugCoordinateSystem(World, ControlPointWorld, DesiredWorldRot.Rotator(),
				18.f, false, 0.f, SDPG_Foreground, 1.5f);

			const float ArrowLen = 28.f;
			const float ArrowThickness = 1.2f;
			// CW / CCW directions use the same per-hand limits as the clamp.
			const FVector CWDir = FQuat(BoneWorldZ, FMath::DegreesToRadians(
				(Hand == EHand::Right) ? -GWristLimitCWDeg : GWristLimitCWDeg))
				.RotateVector(NeutralY);
			const FVector CCWDir = FQuat(BoneWorldZ, FMath::DegreesToRadians(
				(Hand == EHand::Right) ? GWristLimitCCWDeg : -GWristLimitCCWDeg))
				.RotateVector(NeutralY);

			DrawDebugDirectionalArrow(World, ControlPointWorld, ControlPointWorld + NeutralY * ArrowLen,
				8.f, FColor::Green, false, 0.f, SDPG_Foreground, ArrowThickness);
			DrawDebugDirectionalArrow(World, ControlPointWorld, ControlPointWorld + CWDir * ArrowLen,
				8.f, FColor::Cyan, false, 0.f, SDPG_Foreground, ArrowThickness);
			DrawDebugDirectionalArrow(World, ControlPointWorld, ControlPointWorld + CCWDir * ArrowLen,
				8.f, FColor::Yellow, false, 0.f, SDPG_Foreground, ArrowThickness);
		}
	}
}

#pragma endregion


// --------------------------------------------
//      Private - Grip / Transform Helpers
// --------------------------------------------
#pragma region GripTransformHelpers

const FTransform UHeldItemsComponent::GetRelativeTransformBetweenWeaponAndHandBones(EHand Hand) const
{
	const FName WeaponBoneName = GetWeaponBoneName(Hand);
	const FName HandBoneName = GetHandBoneName(Hand);

	// Current animated relative offset (Hand relative to WeaponBone)
	const FTransform WeaponBoneTransform = AnimRefMesh->GetSocketTransform(WeaponBoneName, RTS_World);
	const FTransform HandTransform = AnimRefMesh->GetSocketTransform(HandBoneName, RTS_World);
	return HandTransform.GetRelativeTransform(WeaponBoneTransform);
}

const FTransform UHeldItemsComponent::GetWeaponBoneTransform(EHand Hand) const
{
	if (!AnimRefMesh)
	{
		return FTransform::Identity;
	}
	const FName WeaponBoneName = GetWeaponBoneName(Hand);
	return AnimRefMesh->GetSocketTransform(WeaponBoneName, RTS_World);
}

const FTransform UHeldItemsComponent::GetGripTransform(EHand Hand) const
{
	AItemActor* Item = GetHeldItem(Hand);
	if (!Item || !AnimRefMesh) return FTransform::Identity;

	FTransform Relative = GetRelativeTransformBetweenWeaponAndHandBones(Hand);

	// Apply that relative offset to the current item transform
	// (item is being driven by Physics Control on the WeaponBone)
	FTransform ItemTransform = Item->GetActorTransform();

	// if left hand, flip the X scale to prevent the player's hand from being mirrored
	// (we only want the item mesh to be mirrored)
	if (Hand == EHand::Left)
	{
		FVector Scale = ItemTransform.GetScale3D();
		Scale.X = FMath::Abs(Scale.X);
		ItemTransform.SetScale3D(Scale);
	}

	FTransform GripTransform = Relative * ItemTransform;

	return GripTransform;
}

const FName UHeldItemsComponent::GetWeaponBoneName(EHand Hand) const
{
	if (Hand == EHand::None) return NAME_None;

	return (Hand == EHand::Left) ? TEXT("WeaponBone_L") : TEXT("WeaponBone_R");
}

const FName UHeldItemsComponent::GetHandBoneName(EHand Hand) const
{
	if (Hand == EHand::None) return NAME_None;

	return (Hand == EHand::Left) ? TEXT("Hand_L") : TEXT("Hand_R");
}

#pragma endregion


// --------------------------------------------
//          Protected - Query Helpers
// --------------------------------------------
#pragma region QueryHelpers

AItemActor* UHeldItemsComponent::FindLookedAtItem(float Radius, float MaxDistance) const
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController) return nullptr;

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * MaxDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(GetHeldItem(EHand::Left));
	Params.AddIgnoredActor(GetHeldItem(EHand::Right));

	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

	if (GetWorld()->SweepSingleByChannel(Hit, CameraLocation, TraceEnd, FQuat::Identity,
		TRACE_EQUIPPABLE, Sphere, Params))
	{
		return Cast<AItemActor>(Hit.GetActor());
	}

	return nullptr;
}

bool UHeldItemsComponent::GetIsUnarmed(EHand Hand) const
{
	if (Hand == EHand::None) return false;

	const AItemActor* Item = GetHeldItem(Hand);
	if (!Item || !UnarmedDefinition) return false;

	const UItemInstance* Instance = Item->GetItemInstance();
	return Instance && Instance->ItemDefinition == UnarmedDefinition;
}

FPendingPickupData& UHeldItemsComponent::GetPendingPickup(EHand Hand)
{
	if (Hand == EHand::None)
	{
		static FPendingPickupData Empty;
		return Empty;
	}

	return GetHandState(Hand).PendingPickup;
}

#pragma endregion

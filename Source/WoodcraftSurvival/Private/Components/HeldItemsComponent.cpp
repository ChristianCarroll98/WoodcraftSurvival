// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Components/HeldItemsComponent.h"
#include "Items/ItemActor.h"
#include "Items/ItemFactorySubsystem.h"
#include "Items/ItemInstance.h"
#include "Items/Fragments/EquippableFragment.h"
#include "Core/WoodcraftTypes.h"
#include "Player/FPArmsAnimInstance.h"
#include <PhysicsControlComponent.h>
#include <Engine/AssetManager.h>

UHeldItemsComponent::UHeldItemsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHeldItemsComponent::BeginPlay()
{
	Super::BeginPlay();

	ItemFactory = GetWorld()->GetSubsystem<UItemFactorySubsystem>();

	AnimInstance = Cast<UFPArmsAnimInstance>(AnimRefMesh->GetAnimInstance());

	// Both hands start with Unarmed
	EquipUnarmed(EHand::Left);
	EquipUnarmed(EHand::Right);
}

void UHeldItemsComponent::TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	PreventItemStuck(EHand::Left);
	PreventItemStuck(EHand::Right);
}

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
		// TODO : add drop montage
		bool bSuccess = TryDrop(Hand, OutResult);
		if (!bSuccess) OutResult = TEXT("TryDrop: ") + OutResult;
		return bSuccess;
	}

	// Empty + Unarmed -> nothing
	return false;
}

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

	EquipUnarmed(Hand);
	return true;
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
	
	const UEquippableFragment* EquipFrag = Item->GetItemInstance()->FindFragment<UEquippableFragment>();
	if (!EquipFrag)
	{
		OutResult += TEXT("Could not find ItemEquippableFragment for Item: " + Item->GetName());
		return false;
	}

	// Store pending data first
	FPendingPickupData& Pending = (Hand == EHand::Left) ? PendingPickupLeft : PendingPickupRight;
	Pending.TargetItem = MakeWeakObjectPtr(Item);
	Pending.NeutralPose = EquipFrag->NeutralPose;
	Pending.ExtendedPose = EquipFrag->ExtendedPose;

	bool bSuccess = PlayPickupMontage(Hand, OutResult);
	if (!bSuccess) OutResult = TEXT("PlayPickupMontage: ") + OutResult;

	LoadAndPushPoses(Hand);

	return bSuccess;
}

void UHeldItemsComponent::LoadAndPushPoses(EHand Hand)
{
	const FPendingPickupData& Pending = GetPendingPickup(Hand);

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
	const FPendingPickupData& Pending = GetPendingPickup(Hand);

	UAnimSequence* Neutral = Pending.NeutralPose.Get();
	UAnimSequence* Extended = Pending.ExtendedPose.Get();

	if (!AnimInstance)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
				TEXT("OnPosesLoaded: AnimInstance invalid"));
		return;
	}
	AnimInstance->SetHoldPose(Hand, Neutral, Extended);
}

AItemActor* UHeldItemsComponent::GetHeldItem(EHand Hand) const
{
	if (Hand == EHand::Left) return HeldItemLeft;
	if (Hand == EHand::Right) return HeldItemRight;
	return nullptr;
}

const bool UHeldItemsComponent::GetIsUnarmed(EHand Hand) const
{
	if (Hand == EHand::None) return false;

	const AItemActor* Item = GetHeldItem(Hand);
	if (!Item || !UnarmedDefinition) return false;

	const UItemInstance* Instance = Item->GetItemInstance();
	return Instance && Instance->ItemDefinition == UnarmedDefinition;
}

const FPendingPickupData& UHeldItemsComponent::GetPendingPickup(EHand Hand) const
{
	if (Hand == EHand::None)
	{
		static FPendingPickupData Empty;
		return Empty;
	}

	return (Hand == EHand::Left) ? PendingPickupLeft : PendingPickupRight;
}

EHand UHeldItemsComponent::GetIsHoldingTwoHanded() const
{
	AItemActor* LeftItem = GetHeldItem(EHand::Left);
	if (LeftItem)
	{
		if (const UEquippableFragment* LeftEquippable
			= LeftItem->GetItemInstance()->FindFragment<UEquippableFragment>())
		{
			if (LeftEquippable && LeftEquippable->bTwoHanded) return EHand::Left;
		}
	}

	AItemActor* RightItem = GetHeldItem(EHand::Right);
	if (RightItem)
	{
		if (const UEquippableFragment* RightEquippable
			= RightItem->GetItemInstance()->FindFragment<UEquippableFragment>())
		{
			if (RightEquippable && RightEquippable->bTwoHanded) return EHand::Right;
		}
	}

	return EHand::None;
}

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
	const float EnterUnsafeDistance = 55.0f;
	const float ExitUnsafeDistance = 40.0f;

	bool bItemStuck = GetItemStuck(Hand);

	if (Distance > EnterUnsafeDistance && !bItemStuck)
	{
		SetItemStuck(Hand, true);
		Mesh->SetCollisionResponseToChannel(COLLISION_ITEM, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_STRUCTURE, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_CREATURE, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_HARVESTABLE, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	}
	else if (Distance < ExitUnsafeDistance && bItemStuck)
	{
		SetItemStuck(Hand, false);
		Mesh->SetCollisionResponseToChannel(COLLISION_ITEM, ECR_Block);
		//Mesh->SetCollisionResponseToChannel(COLLISION_STRUCTURE, ECR_Block);
		//Mesh->SetCollisionResponseToChannel(COLLISION_CREATURE, ECR_Block);
		//Mesh->SetCollisionResponseToChannel(COLLISION_HARVESTABLE, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	}
}

const bool UHeldItemsComponent::GetItemStuck(EHand Hand) const
{
	if (Hand == EHand::None) return false;

	return (Hand == EHand::Left) ? bLeftItemStuck : bRightItemStuck;
}

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

void UHeldItemsComponent::SetItemStuck(EHand Hand, bool bStuck)
{
	if (Hand == EHand::None) return;

	if (Hand == EHand::Left) bLeftItemStuck = bStuck;
	else bRightItemStuck = bStuck;
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

bool UHeldItemsComponent::PlayDropMontage(EHand Hand, FString& OutResult) const
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

	TObjectPtr<UAnimMontage> DropMontageToPlay = (Hand == EHand::Left) ?
			DefaultDropMontageLeft : DefaultDropMontageRight;

	if (!DropMontageToPlay)
	{
		OutResult += TEXT("DropMontageToPlay Invalid");
		return false;
	}

	AnimInstance->Montage_Play(DropMontageToPlay);
	return true;
}

void UHeldItemsComponent::GetGripTransforms(FTransform& OutGripTransformLeft, FTransform& OutGripTransformRight) const
{
	OutGripTransformLeft = GetGripTransform(EHand::Left);
	OutGripTransformRight = GetGripTransform(EHand::Right);
}

void UHeldItemsComponent::CompletePickup(EHand Hand)
{
	FString HandStr = Hand == EHand::Left ? "Left" : "Right";
	FString FullString = TEXT("CompletePickup called for hand: ") + HandStr;
	if (GEngine) 

	if (Hand == EHand::None) return;

	// Grab the correct pending data
	const FPendingPickupData& Pending = GetPendingPickup(Hand);

	AItemActor* ItemToPickup = Pending.TargetItem.Get();
	if (!ItemToPickup) return;

	FString OutResult;
	bool bSuccess = TryPickup(ItemToPickup, Hand, OutResult);
	if (!bSuccess) GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Cyan, GErrorPrefix + OutResult);
	return;
	
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

void UHeldItemsComponent::EquipUnarmed(EHand Hand)
{
	if (!UnarmedDefinition || Hand == EHand::None) return;

	// Create new UnarmedActor from definition at current hand transform
	AItemActor* UnarmedActor = ItemFactory->SpawnItemActorFromDefinition(
		UnarmedDefinition,
		GetWeaponBoneTransform(Hand)
	);

	// Visually hide unarmed actor
	UnarmedActor->SetActorHiddenInGame(true);

	FString OutResult;
	bool bSuccess = AttachItemToControl(UnarmedActor, Hand, OutResult);
	if (!bSuccess)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Cyan, GErrorPrefix + OutResult);
		return;
	}
	
	if (!AnimInstance)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Cyan,
				GErrorPrefix + TEXT("EquipUnarmed: AnimInstance invalid"));
		return;
	}
	AnimInstance->SetHoldPose(Hand, UnarmedNeutralPose, UnarmedExtendedPose);
}

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

	const FName WeaponBoneName = GetWeaponBoneName(Hand);

	FPhysicsControlData ControlData;
	ControlData.LinearStrength = 3.0f;
	ControlData.LinearDampingRatio = 1.3f;
	ControlData.AngularStrength = 5.0f;
	ControlData.AngularDampingRatio = 1.2f;
	ControlData.bUseSkeletalAnimation = true;
	ControlData.bDisableCollision = true;
	ControlData.bUseCustomControlPoint = true;
	// --- Get the wrist offset from the weapon bone for the custom control point so the item bends at the wrist
	ControlData.CustomControlPoint = GetRelativeTransformBetweenWeaponAndHandBones(Hand).GetLocation();

	FPhysicsControlTarget ControlTarget;

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

	if (Hand == EHand::Left)
	{
		HeldItemLeft = Item;
		ActiveControlLeft = NewControlName;
	}
	else if (Hand == EHand::Right)
	{
		HeldItemRight = Item;
		ActiveControlRight = NewControlName;
	}
	return true;
}

void UHeldItemsComponent::DetachItemFromControl(EHand Hand)
{
	if (Hand == EHand::None || !PhysicsControl) return;

	FName& ActiveControl = (Hand == EHand::Left) ? ActiveControlLeft : ActiveControlRight;

	if (!ActiveControl.IsNone())
	{
		PhysicsControl->DestroyControl(ActiveControl);
		ActiveControl = NAME_None;
	}


	// Restore collision and scale for the item mesh
	AItemActor* Item = GetHeldItem(Hand);

	if (Item)
	{
		UStaticMeshComponent* ItemMesh = Item->GetItemPrimaryMesh();
		if (ItemMesh)
		{
			if (Hand == EHand::Left)
			{
				ItemMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f)); // Reset scale to normal in case was mirrored for left hand
			}
			ItemMesh->SetCollisionResponseToChannel(COLLISION_PLAYER, ECollisionResponse::ECR_Block);
		}
	}
}

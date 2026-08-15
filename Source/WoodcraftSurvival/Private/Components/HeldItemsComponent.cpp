// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Components/HeldItemsComponent.h"
#include "Items/ItemActor.h"
#include "Items/ItemFactorySubsystem.h"
#include "Items/ItemInstance.h"
#include "Items/Fragments/EquippableFragment.h"
#include "Core/WoodcraftTypes.h"
#include <PhysicsControlComponent.h>

UHeldItemsComponent::UHeldItemsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHeldItemsComponent::BeginPlay()
{
	Super::BeginPlay();

	ItemFactory = GetWorld()->GetSubsystem<UItemFactorySubsystem>();

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

bool UHeldItemsComponent::TryPickupOrDrop(EHand Hand)
{
	if (Hand == EHand::None) return false;

	// Target locked at input time
	AItemActor* LookedAtItem = FindLookedAtItem();
	const bool bHandIsUnarmed = GetIsUnarmed(Hand);

	if (LookedAtItem)
	{
		if (!bHandIsUnarmed) return false; // Occupied + looking at item -> do nothing (swap later)
		
		return TryPickup(LookedAtItem, Hand); // Looking at valid item
	}

	// Looking at empty space
	if (!bHandIsUnarmed)
	{
		return TryDrop(Hand);
	}

	// Empty + Unarmed -> nothing
	return false;
}

bool UHeldItemsComponent::TryPickup(AItemActor* Item, EHand Hand)
{
	if (!Item || Hand == EHand::None) return false;

	// TODO: explicit EquippableFragment + two-handed rules later.

	AItemActor* PreviousItem = GetHeldItem(Hand);

	// Unarmed rule: simply Destroy the previous Unarmed actor
	if (PreviousItem && GetIsUnarmed(Hand))
	{
		PreviousItem->Destroy();
	}

	if (Hand == EHand::Left)
	{
		HeldItemLeft = Item;
	}
	else
	{
		HeldItemRight = Item;
	}

	AttachItemToControl(Item, Hand);

	// TODO: NotifyBecameHeld / montage + AnimNotify finalization
	return true;
}

bool UHeldItemsComponent::TryDrop(EHand Hand)
{
	AItemActor* Item = GetHeldItem(Hand);
	if (Hand == EHand::None || !Item) return false;

	// Never drop Unarmed
	if (GetIsUnarmed(Hand)) return false;

	DetachItemFromControl(Hand);

	EquipUnarmed(Hand);
	return true;
}

AItemActor* UHeldItemsComponent::GetHeldItem(EHand Hand) const
{
	if (Hand == EHand::Left) return HeldItemLeft;
	if (Hand == EHand::Right) return HeldItemRight;
	return nullptr;
}

bool UHeldItemsComponent::IsHolding(EHand Hand) const
{
	AItemActor* Item = GetHeldItem(Hand);
	if (!Item || Item->GetItemInstance()->ItemDefinition == UnarmedDefinition) return false;

	return true;
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

FTransform UHeldItemsComponent::GetRelativeTransformBetweenWeaponAndHandBones(EHand Hand) const
{
	const FName WeaponBoneName = GetWeaponBoneName(Hand);
	const FName HandBoneName = GetHandBoneName(Hand);

	// Current animated relative offset (Hand relative to WeaponBone)
	const FTransform WeaponBoneTransform = AnimRefMesh->GetSocketTransform(WeaponBoneName, RTS_World);
	const FTransform HandTransform = AnimRefMesh->GetSocketTransform(HandBoneName, RTS_World);
	return HandTransform.GetRelativeTransform(WeaponBoneTransform);
}

FTransform UHeldItemsComponent::GetWeaponBoneTransform(EHand Hand) const
{
	if (!AnimRefMesh)
	{
		return FTransform::Identity;
	}
	const FName WeaponBoneName = GetWeaponBoneName(Hand);
	return AnimRefMesh->GetSocketTransform(WeaponBoneName, RTS_World);
}


FTransform UHeldItemsComponent::GetGripTransform(EHand Hand) const
{
	AItemActor* Item = GetHeldItem(Hand);
	if (!Item || !AnimRefMesh) return FTransform::Identity;

	FTransform Relative = GetRelativeTransformBetweenWeaponAndHandBones(Hand);

	// Apply that relative offset to the current item transform
	// (item is being driven by Physics Control on the WeaponBone)
	const FTransform ItemTransform = Item->GetActorTransform();
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
	const FVector GripLocation = GetGripTransform(Hand).GetLocation();

	const float Distance = FVector::Dist(HandBoneLocation, GripLocation);

	// Hysteresis to prevent flickering - TODO: tune later
	const float EnterUnsafeDistance = 55.0f;
	const float ExitUnsafeDistance = 40.0f;

	bool bItemStuck = GetItemStuck(Hand);

	if (Distance > EnterUnsafeDistance && !bItemStuck)
	{
		SetItemStuck(Hand, true);
		//Mesh->SetCollisionResponseToChannel(TRACE_WORLD, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(COLLISION_ITEM, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_CREATURE, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_STRUCTURE, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	}
	else if (Distance < ExitUnsafeDistance && bItemStuck)
	{
		SetItemStuck(Hand, false);
		//Mesh->SetCollisionResponseToChannel(TRACE_WORLD, ECR_Block);
		Mesh->SetCollisionResponseToChannel(COLLISION_ITEM, ECR_Block);
		//Mesh->SetCollisionResponseToChannel(COLLISION_CREATURE, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_STRUCTURE, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	}
}

bool UHeldItemsComponent::GetItemStuck(EHand Hand) const
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

	if (GetWorld()->SweepSingleByChannel(Hit, CameraLocation, TraceEnd, FQuat::Identity, TRACE_EQUIPPABLE, Sphere, Params))
	{
		AItemActor* Item = Cast<AItemActor>(Hit.GetActor());
	    FString ItemName = Item->GetItemInstance()->ItemDefinition->DisplayName.ToString();
	    FString FullString = TEXT("Player sees item: ") + ItemName;
	    if (GEngine) GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Cyan, FullString);
		return Item;
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, TEXT("Player does not see any item"));

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
void UHeldItemsComponent::SetItemStuck(EHand Hand, bool bStuck)
{
	if (Hand == EHand::None) return;

	//FString HandStr = Hand == EHand::Left ? "Left - " : "Right - ";
	//FString StuckStr = bStuck ? TEXT("SetItemStuck set true") : TEXT("SetItemStuck set false");
	//FString FullString = HandStr + StuckStr;
	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Cyan, FullString);

	if (Hand == EHand::Left) bLeftItemStuck = bStuck;
	else bRightItemStuck = bStuck;
}

void UHeldItemsComponent::GetGripTransforms(FTransform& GripTransformLeft, FTransform& GripTransformRight) const
{
	GripTransformLeft = GetGripTransform(EHand::Left);
	GripTransformRight = GetGripTransform(EHand::Right);
}

FName UHeldItemsComponent::GetWeaponBoneName(EHand Hand) const
{
	if (Hand == EHand::None) return NAME_None;

	return (Hand == EHand::Left) ? TEXT("WeaponBone_L") : TEXT("WeaponBone_R");
}

FName UHeldItemsComponent::GetHandBoneName(EHand Hand) const
{
	if (Hand == EHand::None) return NAME_None;

	return (Hand == EHand::Left) ? TEXT("Hand_L") : TEXT("Hand_R");
}

void UHeldItemsComponent::EquipUnarmed(EHand Hand)
{
	if (!UnarmedDefinition || Hand == EHand::None) return;

	AItemActor* UnarmedActor = ItemFactory->SpawnItemActorFromDefinition(
		UnarmedDefinition,
		GetWeaponBoneTransform(Hand) // Spawn at current hand location
	);

	if (!UnarmedActor)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				0,				// Unique key (-1 prevents overwriting, always making a new line)
				5.0f,			// Duration to display the text in seconds
				FColor::Cyan,	// Text color
				TEXT("UnarmedActor reference invalid")	// The text message
			);
		}
		return;
	}

	// Hide the unarmed actor so it doesn't appear in the world. It will still be used for grip transforms and collision.
	UnarmedActor->SetActorHiddenInGame(true);

	if (Hand == EHand::Left) HeldItemLeft = UnarmedActor;
	else HeldItemRight = UnarmedActor;

	AttachItemToControl(UnarmedActor, Hand);
}

void UHeldItemsComponent::AttachItemToControl(AItemActor* Item, EHand Hand)
{
	if (!Item || Hand == EHand::None || !PhysicsControl || !AnimRefMesh) return;

	UStaticMeshComponent* ItemMesh = Item->GetItemPrimaryMesh();
	if (!ItemMesh) return;

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

	if (Hand == EHand::Left) ActiveControlLeft = NewControlName;
	else ActiveControlRight = NewControlName;
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

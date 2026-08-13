// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Components/HeldItemsComponent.h"
#include "Items/ItemActor.h"
#include "Items/ItemDefinition.h"
#include "Items/ItemFactory.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UHeldItemsComponent::UHeldItemsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHeldItemsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Both hands start with Unarmed
	EquipUnarmed(EHand::Left);
	EquipUnarmed(EHand::Right);
}

bool UHeldItemsComponent::TryPickup(AItemActor* Item, EHand Hand)
{
	if (!Item || Hand == EHand::None)
	{
		return false;
	}

	// TODO: Validate that the item has an EquippableFragment
	// TODO: Handle two-handed logic (clear the other hand if necessary)
	// TODO: Play pickup montage / wait for AnimNotify before finalizing

	if (Hand == EHand::Left)
	{
		HeldItemLeft = Item;
	}
	else
	{
		HeldItemRight = Item;
	}

	AttachItemToControl(Item, Hand);

	// TODO: Notify the item that it became held
	// TODO: Hide mesh if this is Unarmed

	return true;
}

bool UHeldItemsComponent::TryDrop(EHand Hand)
{
	if (Hand == EHand::None)
	{
		return false;
	}

	AItemActor* Item = GetHeldItem(Hand);
	if (!Item)
	{
		return false;
	}

	DetachItemFromControl(Hand);

	// TODO: Notify the item that it was released
	// TODO: Return the actor to simulated physics

	EquipUnarmed(Hand);
	return true;
}

AItemActor* UHeldItemsComponent::GetHeldItem(EHand Hand) const
{
	if (Hand == EHand::Left)
	{
		return HeldItemLeft;
	}
	if (Hand == EHand::Right)
	{
		return HeldItemRight;
	}
	return nullptr;
}

bool UHeldItemsComponent::IsHolding(EHand Hand) const
{
	AItemActor* Item = GetHeldItem(Hand);
	if (!Item || !UnarmedDefinition)
	{
		return false;
	}

	// TODO: Better check once ItemInstance is easily accessible
	return true;
}

bool UHeldItemsComponent::IsTwoHanded() const
{
	// TODO: Check the EquippableFragment on the held item
	return false;
}

FTransform UHeldItemsComponent::GetRelativeTransformBetweenWeaponAndHandBones(EHand Hand) const
{
	const FName WeaponBoneName = (Hand == EHand::Left) ? TEXT("WeaponBone_L") : TEXT("WeaponBone_R");
	const FName HandBoneName = (Hand == EHand::Left) ? TEXT("Hand_L") : TEXT("Hand_R");

	// Current animated relative offset (Hand relative to WeaponBone)
	const FTransform WeaponBoneTransform = AnimRefMesh->GetSocketTransform(WeaponBoneName, RTS_World);
	const FTransform HandTransform = AnimRefMesh->GetSocketTransform(HandBoneName, RTS_World);
	return HandTransform.GetRelativeTransform(WeaponBoneTransform);
}


FTransform UHeldItemsComponent::GetGripTransform(EHand Hand) const
{
	AItemActor* Item = GetHeldItem(Hand);
	if (!Item || !AnimRefMesh)
	{
		return FTransform::Identity;
	}

	FTransform Relative = GetRelativeTransformBetweenWeaponAndHandBones(Hand);

	// Apply that relative offset to the current item transform
	// (item is being driven by Physics Control on the WeaponBone)
	const FTransform ItemTransform = Item->GetActorTransform();
	FTransform GripTransform = Relative * ItemTransform;

	return GripTransform;
}

void UHeldItemsComponent::GetGripTransforms(FTransform& GripTransformLeft, FTransform& GripTransformRight) const
{
	GripTransformLeft = GetGripTransform(EHand::Left);
	GripTransformRight = GetGripTransform(EHand::Right);
}

void UHeldItemsComponent::EquipUnarmed(EHand Hand)
{
	if (!UnarmedDefinition || Hand == EHand::None)
	{
		return;
	}

	AItemActor* UnarmedActor = UItemFactory::SpawnItemFromDefinition(
		GetOwner(),
		UnarmedDefinition,
		GetOwner()->GetActorTransform());

	// Hide the unarmed actor so it doesn't appear in the world. It will still be used for grip transforms and collision.
	UnarmedActor->SetHidden(true);

	if (!UnarmedActor)
	{
		return;
	}

	if (Hand == EHand::Left)
	{
		HeldItemLeft = UnarmedActor;
	}
	else
	{
		HeldItemRight = UnarmedActor;
	}

	AttachItemToControl(UnarmedActor, Hand);
}

void UHeldItemsComponent::AttachItemToControl(AItemActor* Item, EHand Hand)
{
	if (!Item || Hand == EHand::None)
	{
		return;
	}

	if (!PhysicsControl || !AnimRefMesh)
	{
		return;
	}

	UStaticMeshComponent* ItemMesh = Item->FindComponentByClass<UStaticMeshComponent>();
	if (!ItemMesh)
	{
		return;
	}

	// Destroy any existing control for this hand first
	DetachItemFromControl(Hand);

	if (Hand == EHand::Left)
	{
		ItemMesh->SetRelativeScale3D(FVector(-1.0f, 1.0f, 1.0f)); // Mirror the item mesh for the left hand
	}

	// Disable collision for the item mesh while it's being held
	ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);

	const FName WeaponBoneName = (Hand == EHand::Left) ? TEXT("WeaponBone_L") : TEXT("WeaponBone_R");

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
		ActiveControlLeft = NewControlName;
	}
	else
	{
		ActiveControlRight = NewControlName;
	}
}

void UHeldItemsComponent::DetachItemFromControl(EHand Hand)
{
	if (Hand == EHand::None)
	{
		return;
	}

	if (!PhysicsControl)
	{
		return;
	}

	FName& ActiveControl = (Hand == EHand::Left) ? ActiveControlLeft : ActiveControlRight;

	if (!ActiveControl.IsNone())
	{
		PhysicsControl->DestroyControl(ActiveControl);
		ActiveControl = NAME_None;
	}


	// Restore collision and scale for the item mesh
	AActor* Item = GetHeldItem(Hand);

	if (Item)
	{
		UStaticMeshComponent* ItemMesh = Item->FindComponentByClass<UStaticMeshComponent>();
		if (ItemMesh)
		{
			if (Hand == EHand::Left)
			{
				ItemMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f)); // Reset scale to normal in case was mirrored for left hand
			}
			ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Block);
		}
	}
}

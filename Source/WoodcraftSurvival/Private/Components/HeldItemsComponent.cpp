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

FTransform UHeldItemsComponent::GetGripTransform(EHand Hand) const
{
	AItemActor* Item = GetHeldItem(Hand);
	if (!Item)
	{
		return FTransform::Identity;
	}

	// Find the mesh that has the grip sockets
	UStaticMeshComponent* MeshComp = Item->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp)
	{
		return Item->GetActorTransform();
	}

	// Default to GripPrimary. Later we can choose GripSecondary for the secondary hand on two-handed items.
	const FName SocketName = TEXT("GripPrimary");

	if (!MeshComp->DoesSocketExist(SocketName))
	{
		return MeshComp->GetComponentTransform();
	}

	FTransform GripTransform = MeshComp->GetSocketTransform(SocketName, RTS_World);

	// Apply left-hand mirror for one-handed items
	if (Hand == EHand::Left)
	{
		//GripTransform = MirrorGripTransform(GripTransform);
	}

	return GripTransform;
}

FTransform UHeldItemsComponent::MirrorGripTransform(const FTransform& Source) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return Source;
	}

	const FTransform OwnerTM = Owner->GetActorTransform();

	// 1. Bring into owner local space
	FTransform Local = Source.GetRelativeTransform(OwnerTM);

	// 2. Mirror position across the character's local YZ plane (negate X)
	FVector MirroredLocation = Local.GetLocation();
	MirroredLocation.X *= -1.0f;

	// 3. Properly mirror the rotation
	//    We reflect the basis vectors across the same plane
	const FQuat Rot = Local.GetRotation();

	FVector Forward = Rot.GetForwardVector();
	FVector Right = Rot.GetRightVector();
	FVector Up = Rot.GetUpVector();

	// Reflect across local YZ plane
	Forward.X *= -1.0f;
	Right.X *= -1.0f;
	// Up.X stays the same for a standard hand mirror

	// Rebuild rotation from mirrored axes
	// Note: Forward = X, Right = Y, Up = Z in Unreal
	FMatrix MirroredMatrix = FRotationMatrix::MakeFromXY(Forward, Right);
	FQuat MirroredRotation = MirroredMatrix.ToQuat();

	// 4. Write back
	Local.SetLocation(MirroredLocation);
	Local.SetRotation(MirroredRotation);

	// 5. Back to world space
	return Local * OwnerTM;
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

	// TODO: Hide the Unarmed mesh
}

void UHeldItemsComponent::AttachItemToControl(AItemActor* Item, EHand Hand)
{
	if (!Item || Hand == EHand::None)
	{
		return;
	}

	UPhysicsControlComponent* PhysControl = (Hand == EHand::Left) ? PhysicsControlLeft : PhysicsControlRight;
	if (!PhysControl || !AnimRefMesh)
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

	// Disable collision for the item mesh while it's being held
	ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);

	const FName BoneName = (Hand == EHand::Left) ? TEXT("WeaponBone_L") : TEXT("WeaponBone_R");
	const FName GripSocketName = TEXT("GripPrimary");

	// --- Get the grip socket location in the item's local space ---
	FVector CustomControlPoint = FVector::ZeroVector;

	if (ItemMesh->DoesSocketExist(GripSocketName))
	{
		// Socket transform relative to the mesh component
		const FTransform SocketTM = ItemMesh->GetSocketTransform(GripSocketName, RTS_Component);
		CustomControlPoint = SocketTM.GetLocation();
	}

	FPhysicsControlData ControlData;
	ControlData.LinearStrength = 5.0f;
	ControlData.LinearDampingRatio = 1.3f;
	ControlData.AngularStrength = 50.0f;
	ControlData.AngularDampingRatio = 1.2f;
	ControlData.bUseSkeletalAnimation = true;
	ControlData.bDisableCollision = true;
	ControlData.bUseCustomControlPoint = true;
	ControlData.CustomControlPoint = CustomControlPoint;

	FPhysicsControlTarget ControlTarget;

	// Create the control using the AnimRef mesh + correct weapon bone
	FName NewControlName = PhysControl->CreateControl(
		AnimRefMesh,
		BoneName,
		ItemMesh,
		NAME_None,
		ControlData,
		ControlTarget,
		TEXT("HeldItems"),
		"PC_"
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

	UPhysicsControlComponent* PhysControl = (Hand == EHand::Left) ? PhysicsControlLeft : PhysicsControlRight;
	if (!PhysControl)
	{
		return;
	}

	FName& ActiveControl = (Hand == EHand::Left) ? ActiveControlLeft : ActiveControlRight;

	if (!ActiveControl.IsNone())
	{
		PhysControl->DestroyControl(ActiveControl);
		ActiveControl = NAME_None;
	}


	// Restore collision for the item mesh
	AActor* Item = GetHeldItem(Hand);

	if (Item)
	{
		UStaticMeshComponent* ItemMesh = Item->FindComponentByClass<UStaticMeshComponent>();
		if (ItemMesh)
		{
			ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Block);
		}
	}
}

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Components/HeldItemsComponent.h"
#include "Items/ItemActor.h"
#include "Items/ItemFactory.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

UHeldItemsComponent::UHeldItemsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHeldItemsComponent::BeginPlay()
{
	Super::BeginPlay();

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

bool UHeldItemsComponent::TryPickup(AItemActor* Item, EHand Hand)
{
	if (!Item || Hand == EHand::None)
	{
		return false;
	}

	// TODO: Validate that the item has an EquippableFragment
	// TODO: Handle two-handed logic (clear the other hand if necessary)
	// TODO: Play pickup montage / wait for AnimNotify before finalizing

	if (Hand == EHand::Left) HeldItemLeft = Item;
	else HeldItemRight = Item;

	AttachItemToControl(Item, Hand);

	// TODO: Notify the item that it became held
	// TODO: Hide mesh if this is Unarmed

	return true;
}

bool UHeldItemsComponent::TryDrop(EHand Hand)
{
	AItemActor* Item = GetHeldItem(Hand);
	if (Hand == EHand::None || !Item) return false;

	DetachItemFromControl(Hand);

	// TODO: Notify the item that it was released
	// TODO: Return the actor to simulated physics

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
	if (!Item || !UnarmedDefinition) return false;

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

	UStaticMeshComponent* Mesh = Item->GetItemMesh();
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
		Mesh->SetCollisionResponseToChannel(COLLISION_WORLD, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(COLLISION_ITEM, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_CREATURE, ECR_Ignore);
		//Mesh->SetCollisionResponseToChannel(COLLISION_STRUCTURE, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	}
	else if (Distance < ExitUnsafeDistance && bItemStuck)
	{
		SetItemStuck(Hand, false);
		Mesh->SetCollisionResponseToChannel(COLLISION_WORLD, ECR_Block);
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

	if (GetWorld()->SweepSingleByChannel(Hit, CameraLocation, TraceEnd, FQuat::Identity, COLLISION_EQUIPPABLE, Sphere, Params))
	{
		return Cast<AItemActor>(Hit.GetActor());
	}
	return nullptr;
}

void UHeldItemsComponent::SetItemStuck(EHand Hand, bool bStuck)
{
	if (Hand == EHand::None) return;

	FString HandStr = Hand == EHand::Left ? "Left - " : "Right - ";
	FString StuckStr = bStuck ? TEXT("SetItemStuck set true") : TEXT("SetItemStuck set false");
	FString FullString = HandStr + StuckStr;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Cyan, FullString);

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

	AItemActor* UnarmedActor = UItemFactory::SpawnItemFromDefinition(
		GetOwner(),
		UnarmedDefinition,
		GetOwner()->GetActorTransform());
	if (!UnarmedActor) return;

	// Hide the unarmed actor so it doesn't appear in the world. It will still be used for grip transforms and collision.
	UnarmedActor->SetHidden(true);
	// Set the transform of the unarmed actor to match the weapon bone for the hand to prevent hands jerking away on spawn.
	UnarmedActor->SetActorTransform(GetWeaponBoneTransform(Hand));

	if (Hand == EHand::Left) HeldItemLeft = UnarmedActor;
	else HeldItemRight = UnarmedActor;

	AttachItemToControl(UnarmedActor, Hand);
}

void UHeldItemsComponent::AttachItemToControl(AItemActor* Item, EHand Hand)
{
	if (!Item || Hand == EHand::None || !PhysicsControl || !AnimRefMesh) return;

	UStaticMeshComponent* ItemMesh = Item->GetItemMesh();
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
		UStaticMeshComponent* ItemMesh = Item->GetItemMesh();
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

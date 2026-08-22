// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemActor.h"
#include "Items/ItemInstance.h"
#include "Items/ItemDefinition.h"
#include "Items/Fragments/ItemFragment.h"
#include "Items/Fragments/DamageItemFragment.h"
#include "Core/WoodcraftTypes.h"
#include <Components/StaticMeshComponent.h>

AItemActor::AItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create the mesh component and make it the root
	PrimaryMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimaryMeshComponent"));
	SetRootComponent(PrimaryMeshComponent);
	PrimaryMeshComponent->SetCollisionProfileName(TEXT("ItemProfile"));
}

UItemInstance* AItemActor::GetItemInstance() const
{
	return ItemInstance;
}

void AItemActor::InitializeFromInstance(UItemInstance* Instance)
{
	if (!Instance || !Instance->ItemDefinition) return;

	ItemInstance = Instance;

	// Load and apply the primary mesh from the definition
	UStaticMesh* PrimaryMesh = Instance->ItemDefinition->PrimaryMesh.LoadSynchronous();
	if (!PrimaryMesh)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			TEXT("Failed to load primary mesh for item: ") + GetName());
		return;
	}

	PrimaryMeshComponent->SetStaticMesh(PrimaryMesh);
	PrimaryMeshComponent->SetSimulatePhysics(true);
	PrimaryMeshComponent->SetUseCCD(true);
	PrimaryMeshComponent->CanCharacterStepUpOn = ECB_No;
	PrimaryMeshComponent->SetLinearDamping(0.05f);
	PrimaryMeshComponent->SetAngularDamping(0.5f);
	
	// Secondary mesh (optional)
	if (!Instance->ItemDefinition->SecondaryMesh.IsNull())
	{
		UStaticMesh* SecondaryMesh = Instance->ItemDefinition->SecondaryMesh.LoadSynchronous();
		
		if (SecondaryMesh)
		{
			SecondaryMeshComponent = NewObject<UStaticMeshComponent>(this, TEXT("SecondaryMeshComponent"));

			SecondaryMeshComponent->SetStaticMesh(SecondaryMesh);
			SecondaryMeshComponent->SetRelativeTransform(Instance->ItemDefinition->SecondaryRelativeTransform);
			SecondaryMeshComponent->SetCollisionProfileName(TEXT("Item"));

			SecondaryMeshComponent->SetupAttachment(PrimaryMeshComponent);
			SecondaryMeshComponent->RegisterComponent();

			SecondaryMeshComponent->SetSimulatePhysics(true);
			SecondaryMeshComponent->SetUseCCD(true);
			SecondaryMeshComponent->CanCharacterStepUpOn = ECB_No;
			SecondaryMeshComponent->SetLinearDamping(0.05f);
			SecondaryMeshComponent->SetAngularDamping(0.5f);

			SecondaryMeshComponent->WeldTo(PrimaryMeshComponent);
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
				TEXT("Failed to load secondary mesh for item. Skipping... item: ") + GetName());
		}
	}

	PrimaryMeshComponent->RecreatePhysicsState();

	// Fragments run after base mesh/physics setup.
	// UDamageItemFragment::OnItemSpawned enables collision hit events if present.
	for (UItemFragment* Fragment : Instance->ItemDefinition->Fragments)
	{
		if (Fragment) Fragment->OnItemSpawned(this);
	}
}

UStaticMeshComponent* AItemActor::GetItemPrimaryMesh() const
{
	return PrimaryMeshComponent;
}

UStaticMeshComponent* AItemActor::GetItemSecondaryMesh() const
{
	return SecondaryMeshComponent;
}

FTransform AItemActor::GetSecondaryRelativeTransform() const
{
	return ItemInstance->ItemDefinition->SecondaryRelativeTransform;
}

void AItemActor::ApplyDamage(const FDamageInfo& DamageInfo)
{
	// No-op for now. Durability / resource-item breakdown will live here later.
}

void AItemActor::EnableCollisionDamage(UStaticMeshComponent* Mesh)
{
	if (!Mesh) return;

	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->OnComponentHit.AddDynamic(this, &AItemActor::OnItemMeshHit);
}

void AItemActor::OnItemMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// Only do damage if this item is held (for now)
	if (!Holder.IsValid()) return;

	if (!OtherActor || OtherActor == this) return;

	// Never damage the actor currently holding this item
	if (OtherActor == Holder.Get()) return;

	// Never damage another held item (player's own sticks, etc.), later handle multiplayer (held by other player)
	if (AItemActor* OtherItem = Cast<AItemActor>(OtherActor))
	{
		if (OtherItem->Holder.IsValid()) return;
	}

	// Relative speed gate
	if (NormalImpulse.Size() < GMinImpulse) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDamageTime < DamageCooldown) return;

	// Need a damage fragment to deal anything
	if (!ItemInstance) return;
	const UDamageItemFragment* DamageFrag = ItemInstance->FindFragment<UDamageItemFragment>();
	if (!DamageFrag || DamageFrag->BaseDamage <= 0.f) return;

	// Only continue if the target can take damage
	IDamageable* Damageable = Cast<IDamageable>(OtherActor);
	if (!Damageable) return;

	// Simple linear scale for now (can become a curve or per-DamageType later)
	const float SpeedScale = FMath::Clamp(NormalImpulse.Size() / 500.f, 0.1f, 2.0f);
	const float Amount = DamageFrag->BaseDamage * SpeedScale;

	FDamageInfo Info;
	Info.Amount = Amount;
	Info.DamageType = DamageFrag->DamageType;
	Info.HitLocation = Hit.ImpactPoint;
	Info.Instigator = Holder.IsValid() ? Holder.Get() : this;

	Damageable->ApplyDamage(Info);

	LastDamageTime = Now;
}

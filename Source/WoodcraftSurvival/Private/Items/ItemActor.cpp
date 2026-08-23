// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemActor.h"
#include "Items/ItemInstance.h"
#include "Items/ItemDefinition.h"
#include "Items/Fragments/ItemFragment.h"
#include "Items/Fragments/DamageItemFragment.h"
#include "Items/Fragments/EquippableItemFragment.h"
#include "Player/HeldItemsComponent.h"
#include "Core/WoodcraftTypes.h"
#include "Core/Damage/DamageType_Blunt.h"
#include "Core/Damage/DamageType_Slash.h"
#include "Core/Damage/DamageType_Pierce.h"
#include <Components/StaticMeshComponent.h>
#include <PhysicsEngine/PhysicsConstraintComponent.h>
#include <PhysicsEngine/BodySetup.h>

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
			SecondaryMeshComponent->SetCollisionProfileName(TEXT("ItemProfile"));

			SecondaryMeshComponent->SetupAttachment(PrimaryMeshComponent);
			SecondaryMeshComponent->RegisterComponent();

			SecondaryMeshComponent->SetSimulatePhysics(true);
			SecondaryMeshComponent->SetUseCCD(true);
			SecondaryMeshComponent->CanCharacterStepUpOn = ECB_No;
			SecondaryMeshComponent->SetLinearDamping(0.05f);
			SecondaryMeshComponent->SetAngularDamping(0.5f);

			// Fixed constraint = rigid connection (Chaos-friendly dual-mesh)
			UPhysicsConstraintComponent* Constraint = NewObject<UPhysicsConstraintComponent>(this,
				TEXT("SecondaryConstraint"));
			Constraint->SetupAttachment(PrimaryMeshComponent);
			Constraint->RegisterComponent();
			Constraint->SetConstrainedComponents(PrimaryMeshComponent, NAME_None, SecondaryMeshComponent,
				NAME_None);
			Constraint->SetDisableCollision(true);

			Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
			Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
			Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
			Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
			Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
			Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.f);
		}
	}

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

TSubclassOf<UDamageType> AItemActor::ResolveDamageTypeFromShapeName(FName ShapeName)
{
	if (ShapeName.IsNone())
	{
		return UDamageType_Blunt::StaticClass();
	}

	const FString Name = ShapeName.ToString().ToLower();

	if (Name.Contains(TEXT("slash")) || Name.Contains(TEXT("sharp")) || Name.Contains(TEXT("blade")))
	{
		return UDamageType_Slash::StaticClass();
	}

	if (Name.Contains(TEXT("pierce")) || Name.Contains(TEXT("point")))
	{
		return UDamageType_Pierce::StaticClass();
	}

	// Blunt, Head, Handle, or any unrecognised name
	return UDamageType_Blunt::StaticClass();
}

void AItemActor::OnItemMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// Only do damage if this item is held (for now)
	if (!Holder.IsValid()) return;

	// Must be extended (strike-ready) on the hand that holds this item
	if (UHeldItemsComponent* HeldComp = Holder->FindComponentByClass<UHeldItemsComponent>())
	{
		const EHand Hand = HeldComp->GetHandHoldingItem(this);
		if (Hand == EHand::None || !HeldComp->GetIsExtended(Hand)) return;
	}
	else
	{
		return;
	}

	if (!OtherActor || OtherActor == this) return;

	// Never damage the actor currently holding this item
	if (OtherActor == Holder.Get()) return;

	// Never damage another held item (player's own sticks, etc.), later handle multiplayer (held by other player)
	if (AItemActor* OtherItem = Cast<AItemActor>(OtherActor))
	{
		if (OtherItem->Holder.IsValid()) return;
	}

	// Impulse + actual motion gates (stops “lean on the tree and drain health”)
	if (NormalImpulse.Size() < GMinImpulse) return;
	if (HitComp && HitComp->GetPhysicsLinearVelocity().Size() < GMinItemSpeed) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDamageTime < DamageCooldown) return;

	// Need the enable-hits marker fragment
	if (!ItemInstance) return;
	const UDamageItemFragment* DamageFrag = ItemInstance->FindFragment<UDamageItemFragment>();
	if (!DamageFrag) return;

	// Only continue if the target can take damage
	IDamageable* Damageable = Cast<IDamageable>(OtherActor);
	if (!Damageable) return;

	// Extract the named collision primitive on *this* item (the part that contacted)
	FName ShapeName = NAME_None;
	if (HitComp)
	{
		if (UBodySetup* BodySetup = HitComp->GetBodySetup())
		{
			// MyItem = shape index on the component that generated the hit event
			const int32 ShapeIndex = (Hit.MyItem >= 0) ? Hit.MyItem : static_cast<int32>(Hit.ElementIndex);
			if (const FKShapeElem* Elem = BodySetup->AggGeom.GetElement(ShapeIndex))
			{
				ShapeName = Elem->GetName();
			}
		}
	}

	const TSubclassOf<UDamageType> CandidateType = ResolveDamageTypeFromShapeName(ShapeName);
	TSubclassOf<UDamageType> FinalType = CandidateType;
	FString ConversionReason = TEXT("kept");

	// Incidence / angle → type conversion (model B)
	// Shape name decides the candidate; StrikeMode decides which local axis must align with velocity.
	if (CandidateType == UDamageType_Slash::StaticClass() || CandidateType == UDamageType_Pierce::StaticClass())
	{
		const UEquippableItemFragment* EquipFrag = ItemInstance->FindFragment<UEquippableItemFragment>();
		const EItemStrikeMode Mode = EquipFrag ? EquipFrag->StrikeMode : EItemStrikeMode::None;

		const FVector Velocity = HitComp ? HitComp->GetPhysicsLinearVelocity() : FVector::ZeroVector;
		const float Speed = Velocity.Size();

		if (Speed > KINDA_SMALL_NUMBER && HitComp)
		{
			const FVector VelDir = Velocity / Speed;
			const FTransform MeshXform = HitComp->GetComponentTransform();

			bool bKeep = false;
			float AngleDeg = 180.f;
			FString AxisUsed = TEXT("none");

			if (CandidateType == UDamageType_Slash::StaticClass())
			{
				if (Mode == EItemStrikeMode::SingleEdged)
				{
					// Prefer +Y only
					const FVector StrikeDir = MeshXform.GetUnitAxis(EAxis::Y);
					const float Dot = FVector::DotProduct(VelDir, StrikeDir);
					AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
					bKeep = (AngleDeg <= GSlashMaxAngleDeg);
					AxisUsed = TEXT("+Y");
				}
				else if (Mode == EItemStrikeMode::DoubleEdged)
				{
					// ±Y (Abs)
					const FVector StrikeDir = MeshXform.GetUnitAxis(EAxis::Y);
					const float AbsDot = FMath::Abs(FVector::DotProduct(VelDir, StrikeDir));
					AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(AbsDot, -1.f, 1.f)));
					bKeep = (AngleDeg <= GSlashMaxAngleDeg);
					AxisUsed = TEXT("±Y");
				}
				// else Mode does not support Slash → force Blunt
			}
			else // Pierce candidate
			{
				if (Mode == EItemStrikeMode::Pierce)
				{
					// +Z only (tip)
					const FVector StrikeDir = MeshXform.GetUnitAxis(EAxis::Z);
					const float Dot = FVector::DotProduct(VelDir, StrikeDir);
					AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
					bKeep = (AngleDeg <= GPierceMaxAngleDeg);
					AxisUsed = TEXT("+Z");
				}
			}

			if (bKeep)
			{
				ConversionReason = FString::Printf(TEXT("%s kept (%.0f° on %s)"),
					CandidateType == UDamageType_Slash::StaticClass() ? TEXT("Slash") : TEXT("Pierce"),
					AngleDeg, *AxisUsed);
			}
			else
			{
				FinalType = UDamageType_Blunt::StaticClass();
				ConversionReason = FString::Printf(TEXT("%s→Blunt (%.0f° on %s, mode=%d)"),
					CandidateType == UDamageType_Slash::StaticClass() ? TEXT("Slash") : TEXT("Pierce"),
					AngleDeg, *AxisUsed, static_cast<int32>(Mode));
			}
		}
		else
		{
			FinalType = UDamageType_Blunt::StaticClass();
			ConversionReason = TEXT("no velocity → Blunt");
		}
	}

	// Amount from impulse only (mass already contributes via physics / PhysMats)
	const float ImpulseMag = NormalImpulse.Size();
	const float Amount = FMath::Clamp(ImpulseMag * 0.01f, 0.5f, 40.f);

	FDamageInfo Info;
	Info.Amount = Amount;
	Info.DamageType = FinalType;
	Info.HitLocation = Hit.ImpactPoint;
	Info.Instigator = Holder.IsValid() ? Holder.Get() : this;

	Damageable->ApplyDamage(Info);

	LastDamageTime = Now;

	// Debug (remove or gate later)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("Hit %s | Shape=%s → %s | %s | Impulse=%.0f Amount=%.1f"),
				*OtherActor->GetName(),
				*ShapeName.ToString(),
				FinalType ? *FinalType->GetName() : TEXT("None"),
				*ConversionReason,
				ImpulseMag,
				Amount));
	}
}

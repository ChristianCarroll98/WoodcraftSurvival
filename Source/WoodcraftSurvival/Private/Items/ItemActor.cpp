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
#include <DrawDebugHelpers.h>

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

/**
 * Chaos frequently returns MyItem = -1 for multi-shape simple collision.
 * When that happens we find the named shape whose geometry is closest to the impact point.
 * Supports Box / Sphere / Sphyl (capsule); other types fall back to center distance.
 */
static FName ResolveShapeNameFromHit(UPrimitiveComponent* HitComp, const FHitResult& Hit)
{
	if (!HitComp) return NAME_None;

	UBodySetup* BodySetup = HitComp->GetBodySetup();
	if (!BodySetup) return NAME_None;

	const FTransform CompTM = HitComp->GetComponentTransform();
	const FVector LocalImpact = CompTM.InverseTransformPosition(Hit.ImpactPoint);

	float BestDistSq = TNumericLimits<float>::Max();
	FName BestName = NAME_None;

	auto Consider = [&](FName Name, float DistSq)
	{
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestName = Name;
		}
	};

	// --- Boxes ---
	for (const FKBoxElem& Box : BodySetup->AggGeom.BoxElems)
	{
		// Box local space (Center + Rotation)
		const FTransform BoxTM(Box.Rotation, Box.Center);
		const FVector BoxLocal = BoxTM.InverseTransformPosition(LocalImpact);

		const FVector Extents(Box.X * 0.5f, Box.Y * 0.5f, Box.Z * 0.5f);
		const FVector Clamped(
			FMath::Clamp(BoxLocal.X, -Extents.X, Extents.X),
			FMath::Clamp(BoxLocal.Y, -Extents.Y, Extents.Y),
			FMath::Clamp(BoxLocal.Z, -Extents.Z, Extents.Z));

		const float DistSq = FVector::DistSquared(BoxLocal, Clamped);
		Consider(Box.GetName(), DistSq);
	}

	// --- Spheres ---
	for (const FKSphereElem& Sphere : BodySetup->AggGeom.SphereElems)
	{
		const float Dist = FVector::Dist(LocalImpact, Sphere.Center) - Sphere.Radius;
		const float DistSq = FMath::Square(FMath::Max(0.f, Dist));
		Consider(Sphere.GetName(), DistSq);
	}

	// --- Capsules (Sphyl) ---
	for (const FKSphylElem& Cap : BodySetup->AggGeom.SphylElems)
	{
		const FTransform CapTM(Cap.Rotation, Cap.Center);
		const FVector CapLocal = CapTM.InverseTransformPosition(LocalImpact);

		// Capsule is along Z, half-length = Length*0.5, radius = Radius
		const float HalfLen = Cap.Length * 0.5f;
		const float ClampedZ = FMath::Clamp(CapLocal.Z, -HalfLen, HalfLen);
		const FVector ClosestOnAxis(0.f, 0.f, ClampedZ);
		const float Dist = FVector::Dist(CapLocal, ClosestOnAxis) - Cap.Radius;
		const float DistSq = FMath::Square(FMath::Max(0.f, Dist));
		Consider(Cap.GetName(), DistSq);
	}

	// --- Convex / other: fall back to element center if any remain ---
	// (GetElement walks all types; we already handled the common ones above, so this is a safety net)
	const int32 Num = BodySetup->AggGeom.GetElementCount();
	for (int32 i = 0; i < Num; ++i)
	{
		if (const FKShapeElem* Elem = BodySetup->AggGeom.GetElement(i))
		{
			// Skip if we already considered it via the typed arrays (name match is fine; distance will just re-evaluate)
			// For true unknown types we only have the generic interface, so use a large penalty unless it is the only shape.
			// Simple center approximation using the first available transform-like data is not exposed generically,
			// so we only use this path when no better candidate was found.
			if (BestName.IsNone())
			{
				Consider(Elem->GetName(), 0.f); // accept first unknown as last resort
			}
		}
	}

	return BestName;
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

	// Named collision primitive that contacted (Chaos often returns MyItem = -1, so we use
	// closest-shape against ImpactPoint when the index is unusable).
	const FName ShapeName = HitComp ? ResolveShapeNameFromHit(HitComp, Hit) : NAME_None;
	const TSubclassOf<UDamageType> CandidateType = ResolveDamageTypeFromShapeName(ShapeName);
	TSubclassOf<UDamageType> FinalType = CandidateType;

	// Incoming direction for incidence + debug (pre-bounce velocity preferred)
	FVector IncomingDir = FVector::ZeroVector;
	if (Holder.IsValid())
	{
		if (UHeldItemsComponent* HeldComp = Holder->FindComponentByClass<UHeldItemsComponent>())
		{
			const EHand Hand = HeldComp->GetHandHoldingItem(this);
			const FVector LastVel = HeldComp->GetLastItemVelocity(Hand);
			if (LastVel.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				IncomingDir = LastVel.GetSafeNormal();
			}
		}
	}
	if (IncomingDir.IsNearlyZero() && HitComp)
	{
		const FVector LiveVel = HitComp->GetPhysicsLinearVelocity();
		if (LiveVel.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			IncomingDir = LiveVel.GetSafeNormal();
		}
	}
	if (IncomingDir.IsNearlyZero() && NormalImpulse.Size() > KINDA_SMALL_NUMBER)
	{
		IncomingDir = -NormalImpulse.GetSafeNormal();
	}

	// Preferred strike axis for incidence + debug
	FVector StrikeDir = FVector::ZeroVector;
	const UEquippableItemFragment* EquipFrag = ItemInstance->FindFragment<UEquippableItemFragment>();
	const EItemStrikeMode Mode = EquipFrag ? EquipFrag->StrikeMode : EItemStrikeMode::None;
	if (HitComp)
	{
		const FTransform MeshXform = HitComp->GetComponentTransform();
		if (Mode == EItemStrikeMode::Pierce)
		{
			StrikeDir = MeshXform.GetUnitAxis(EAxis::Z);
		}
		else if (Mode == EItemStrikeMode::SingleEdged || Mode == EItemStrikeMode::DoubleEdged)
		{
			StrikeDir = MeshXform.GetUnitAxis(EAxis::Y);
		}
	}

	// Incidence (model B): shape name decides the candidate; StrikeMode decides which local
	// axis must align with the incoming direction.
	if (CandidateType == UDamageType_Slash::StaticClass() || CandidateType == UDamageType_Pierce::StaticClass())
	{
		bool bKeep = false;
		if (!IncomingDir.IsNearlyZero() && !StrikeDir.IsNearlyZero())
		{
			if (CandidateType == UDamageType_Slash::StaticClass())
			{
				if (Mode == EItemStrikeMode::SingleEdged)
				{
					const float AngleDeg = FMath::RadiansToDegrees(
						FMath::Acos(FMath::Clamp(FVector::DotProduct(IncomingDir, StrikeDir), -1.f, 1.f)));
					bKeep = (AngleDeg <= GSlashMaxAngleDeg);
				}
				else if (Mode == EItemStrikeMode::DoubleEdged)
				{
					const float AngleDeg = FMath::RadiansToDegrees(
						FMath::Acos(FMath::Clamp(FMath::Abs(FVector::DotProduct(IncomingDir, StrikeDir)), -1.f, 1.f)));
					bKeep = (AngleDeg <= GSlashMaxAngleDeg);
				}
			}
			else // Pierce candidate
			{
				if (Mode == EItemStrikeMode::Pierce)
				{
					const float AngleDeg = FMath::RadiansToDegrees(
						FMath::Acos(FMath::Clamp(FVector::DotProduct(IncomingDir, StrikeDir), -1.f, 1.f)));
					bKeep = (AngleDeg <= GPierceMaxAngleDeg);
				}
			}
		}

		if (!bKeep)
		{
			FinalType = UDamageType_Blunt::StaticClass();
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

	// Debug: incoming swing dir (cyan) vs preferred edge axis (yellow), 5s, foreground
	if (UWorld* World = GetWorld())
	{
		const FVector Loc = Hit.ImpactPoint;
		const float Len = 40.f;
		const float ArrowSize = 12.f;
		const float Life = 5.f;
		const uint8 DepthPri = SDPG_Foreground;
		if (!IncomingDir.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(World, Loc, Loc + IncomingDir * Len, ArrowSize,
				FColor::Cyan, false, Life, DepthPri, 2.f);
		}
		if (!StrikeDir.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(World, Loc, Loc + StrikeDir * Len, ArrowSize,
				FColor::Yellow, false, Life, DepthPri, 2.f);
		}
	}

	if (GEngine)
	{
		const FString TypeName = FinalType ? FinalType->GetName() : TEXT("None");
		GEngine->AddOnScreenDebugMessage(
			42, // fixed ID → overwrites previous hit line
			2.5f,
			FColor::MakeRandomColor(),
			FString::Printf(TEXT("Hit shape=%s  type=%s  amt=%.1f"),
				*ShapeName.ToString(), *TypeName, Amount));
	}

	Damageable->ApplyDamage(Info);
	LastDamageTime = Now;
}

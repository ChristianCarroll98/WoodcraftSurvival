// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Harvestables/HarvestableActor.h"
#include "Harvestables/HarvestableDefinition.h"
#include "Harvestables/HarvestableInstance.h"
#include "Harvestables/Fragments/HarvestableFragment.h"
#include "Harvestables/Fragments/HealthHarvestableFragment.h"
#include "Harvestables/Fragments/YieldHarvestableFragment.h"
#include "Harvestables/HarvestableFactorySubsystem.h"
#include "Items/ItemFactorySubsystem.h"
#include "Items/ItemDefinition.h"
#include <Components/StaticMeshComponent.h>
#include "Engine/StaticMesh.h"

AHarvestableActor::AHarvestableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PrimaryMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimaryMesh"));
	SetRootComponent(PrimaryMeshComponent);

	// Standing harvestables start kinematic / non-simulating.
	// Physics simulation is only enabled later (e.g. FallableHarvestableFragment or when becoming an Item).
	PrimaryMeshComponent->SetSimulatePhysics(false);
	PrimaryMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PrimaryMeshComponent->SetCollisionProfileName(TEXT("HarvestableProfile"));
}

void AHarvestableActor::BeginPlay()
{
	Super::BeginPlay();
}

void AHarvestableActor::InitializeFromDefinition(UHarvestableDefinition* InDefinition)
{
	if (!InDefinition)
	{
		return;
	}

	Definition = InDefinition;
	Instance = nullptr; // pure Definition path – Instance created lazily later

	SetupMeshFromDefinition();
	NotifyFragmentsSpawned();
}

void AHarvestableActor::InitializeFromInstance(UHarvestableInstance* InInstance)
{
	if (!InInstance || !InInstance->Definition)
	{
		return;
	}

	Instance = InInstance;
	Definition = InInstance->Definition;

	SetupMeshFromDefinition();
	NotifyFragmentsSpawned();
}

void AHarvestableActor::PromoteToInstance()
{
	if (Instance || !Definition)
	{
		return; // already has one, or no definition
	}

	if (UWorld* World = GetWorld())
	{
		if (UHarvestableFactorySubsystem* Factory = World->GetSubsystem<UHarvestableFactorySubsystem>())
		{
			Instance = Factory->CreateInstanceFromDefinition(Definition);
			// OnHarvestableInstanceCreated has already been called inside the Factory.
		}
	}
}

void AHarvestableActor::SetupMeshFromDefinition()
{
	if (!Definition || !PrimaryMeshComponent)
	{
		return;
	}

	if (UStaticMesh* Mesh = Definition->PrimaryMesh.LoadSynchronous())
	{
		PrimaryMeshComponent->SetStaticMesh(Mesh);
	}
	else
	{
		// Optional debug – remove or gate behind a CVar later
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
				FString::Printf(TEXT("HarvestableActor: No PrimaryMesh on Definition %s"), *GetNameSafe(Definition)));
		}
	}
}

void AHarvestableActor::NotifyFragmentsSpawned()
{
	if (!Definition)
	{
		return;
	}

	for (UHarvestableFragment* Fragment : Definition->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnHarvestableSpawned(this);
		}
	}
}

void AHarvestableActor::ApplyDamage(const FDamageInfo& DamageInfo)
{
	if (DamageInfo.Amount <= 0.f) return;

	// First meaningful interaction → create the long-lived Instance
	if (!HasInstance())
	{
		PromoteToInstance();
	}

	if (!Instance) return; // promote failed (no Definition or no World)

	const float FinalAmount = DamageInfo.Amount * Instance->GetDamageMultiplier(DamageInfo.DamageType);
	if (FinalAmount <= 0.f) return;

	Instance->CurrentHealth -= FinalAmount;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage
		(
			-1,
			3.0f,
			FColor::Orange,
			FString::Printf(TEXT("%s health: %.1f (raw %.1f → final %.1f)"),
				*GetNameSafe(this), Instance->CurrentHealth, DamageInfo.Amount, FinalAmount)
		);
	}

	if (Instance->CurrentHealth <= 0.f)
	{
		HandleDeath();
	}
}

void AHarvestableActor::HandleDeath()
{
	if (!Definition)
	{
		Destroy();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	const FTransform SpawnTransform = GetActorTransform();

	// Center of the mesh is a better origin for scattered items (sticks, rocks, etc.)
	FVector ItemSpawnOrigin = SpawnTransform.GetLocation();
	if (PrimaryMeshComponent)
	{
		ItemSpawnOrigin = PrimaryMeshComponent->Bounds.Origin;
	}

	if (const UYieldHarvestableFragment* YieldFrag = Definition->FindFragment<UYieldHarvestableFragment>())
	{
		// ----- Yield Items -----
		if (UItemFactorySubsystem* ItemFactory = World->GetSubsystem<UItemFactorySubsystem>())
		{
			for (const FHarvestableYieldEntry& Entry : YieldFrag->Yields)
			{
				UItemDefinition* ItemDef = Entry.ItemDefinition.LoadSynchronous();
				if (!ItemDef || Entry.Count <= 0) continue;

				for (int32 i = 0; i < Entry.Count; ++i)
				{
					FVector Offset(
						FMath::FRandRange(-40.f, 40.f),
						FMath::FRandRange(-40.f, 40.f),
						FMath::FRandRange(-20.f, 30.f)
					);

					FTransform ItemTransform;
					ItemTransform.SetLocation(ItemSpawnOrigin + Offset);
					ItemTransform.SetRotation(FQuat(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f)));

					ItemFactory->SpawnItemActorFromDefinition(ItemDef, ItemTransform);
				}
			}
		}

		// ----- Optional Replacement Harvestable (stump / next stage) -----
		// Stays at the Actor transform so it sits correctly on the ground.
		if (!YieldFrag->ReplacementHarvestable.IsNull())
		{
			if (UHarvestableDefinition* ReplacementDef = YieldFrag->ReplacementHarvestable.LoadSynchronous())
			{
				if (UHarvestableFactorySubsystem* HarvestFactory = World->GetSubsystem<UHarvestableFactorySubsystem>())
				{
					HarvestFactory->SpawnActorFromDefinition(ReplacementDef, SpawnTransform);
				}
			}
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage
		(
			-1,
			5.0f,
			FColor::Red,
			FString::Printf(TEXT("%s destroyed – yields spawned"), *GetNameSafe(this))
		);
	}

	// Instance will be collected once no longer referenced.
	// Explicit cleanup can be added later if we keep a registry.
	Destroy();
}
// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Harvestables/HarvestableActor.h"
#include "Harvestables/HarvestableDefinition.h"
#include "Harvestables/HarvestableInstance.h"
#include "Harvestables/Fragments/HarvestableFragment.h"
#include "Harvestables/HarvestableFactorySubsystem.h"
#include "Components/StaticMeshComponent.h"
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

	Instance->CurrentHealth -= DamageInfo.Amount;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage
		(
			-1,
			3.0f,
			FColor::Orange,
			FString::Printf(TEXT("%s health: %.1f"), *GetNameSafe(this), Instance->CurrentHealth)
		);
	}

	if (Instance->CurrentHealth <= 0.f)
	{
		HandleDeath();
	}
}

void AHarvestableActor::HandleDeath()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage
		(
			-1,
			5.0f,
			FColor::Red,
			FString::Printf(TEXT("%s destroyed (death)"), *GetNameSafe(this))
		);
	}

	// Temporary: just destroy the actor.
	// Next step will spawn sticks + stump via Yield fragment before destroying.
	Destroy();
}
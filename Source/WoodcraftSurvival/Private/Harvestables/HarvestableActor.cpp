// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "HarvestableActor.h"
#include "HarvestableDefinition.h"
#include "HarvestableInstance.h"
#include "HarvestableFragment.h"
#include "HarvestableFactorySubsystem.h"
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
	ApplyBaseCollision();
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
	ApplyBaseCollision();
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
			// OnInstanceCreated has already been called inside the Factory.
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

void AHarvestableActor::ApplyBaseCollision()
{
	if (!PrimaryMeshComponent)
	{
		return;
	}

	// Hybrid pattern (locked): apply the shared "Harvestable" profile first,
	// then let fragments override individual responses in OnHarvestableSpawned.
	PrimaryMeshComponent->SetCollisionProfileName(TEXT("Harvestable"));
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

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "HarvestableFactorySubsystem.h"
#include "HarvestableDefinition.h"
#include "HarvestableInstance.h"
#include "HarvestableFragment.h"
#include "HarvestableActor.h"
#include "Engine/World.h"

UHarvestableInstance* UHarvestableFactorySubsystem::CreateInstanceFromDefinition(UHarvestableDefinition* Definition)
{
	if (!Definition)
	{
		return nullptr;
	}

	UHarvestableInstance* Instance = NewObject<UHarvestableInstance>(GetTransientPackage());
	Instance->Definition = Definition;

	// Let every fragment write its default runtime values
	for (UHarvestableFragment* Fragment : Definition->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnHarvestableInstanceCreated(Instance);
		}
	}

	return Instance;
}

AHarvestableActor* UHarvestableFactorySubsystem::SpawnActorFromDefinition(UHarvestableDefinition* Definition, const FTransform& Transform)
{
	if (!Definition || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AHarvestableActor* Actor = GetWorld()->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), Transform, SpawnParams);
	if (Actor)
	{
		Actor->InitializeFromDefinition(Definition);
	}

	return Actor;
}

AHarvestableActor* UHarvestableFactorySubsystem::SpawnActorFromInstance(UHarvestableInstance* Instance, const FTransform& Transform)
{
	if (!Instance || !Instance->Definition || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AHarvestableActor* Actor = GetWorld()->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), Transform, SpawnParams);
	if (Actor)
	{
		Actor->InitializeFromInstance(Instance);
	}

	return Actor;
}

void UHarvestableFactorySubsystem::PromoteToInstance(AHarvestableActor* Actor)
{
	if (!Actor || Actor->HasInstance())
	{
		return;
	}

	// Actor already has the convenience method that does the same work
	Actor->PromoteToInstance();
}

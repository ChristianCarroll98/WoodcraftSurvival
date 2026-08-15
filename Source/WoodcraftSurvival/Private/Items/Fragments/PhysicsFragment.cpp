// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/PhysicsFragment.h"
#include "Items/ItemActor.h"

void UPhysicsFragment::OnItemSpawned(AItemActor* ItemActor)
{
	if (!ItemActor) return;
    
    UStaticMeshComponent* PrimaryMeshComponent = ItemActor->GetItemPrimaryMesh();
	if (!IsValid(PrimaryMeshComponent) || PrimaryMeshComponent->GetStaticMesh() == nullptr) return;

    PrimaryMeshComponent->SetSimulatePhysics(true);
	PrimaryMeshComponent->SetUseCCD(true);
	PrimaryMeshComponent->CanCharacterStepUpOn = ECB_No;
	PrimaryMeshComponent->SetLinearDamping(0.05f);
	PrimaryMeshComponent->SetAngularDamping(0.5f);

	UStaticMeshComponent* SecondaryMeshComponent = ItemActor->GetItemSecondaryMesh();
	if (IsValid(SecondaryMeshComponent) && SecondaryMeshComponent->GetStaticMesh() != nullptr)
	{
		SecondaryMeshComponent->SetSimulatePhysics(true);
		SecondaryMeshComponent->SetUseCCD(true);
		SecondaryMeshComponent->WeldTo(PrimaryMeshComponent);
		SecondaryMeshComponent->CanCharacterStepUpOn = ECB_No;
		SecondaryMeshComponent->SetLinearDamping(0.05f);
		SecondaryMeshComponent->SetAngularDamping(0.5f);
	}

	PrimaryMeshComponent->RecreatePhysicsState();
}

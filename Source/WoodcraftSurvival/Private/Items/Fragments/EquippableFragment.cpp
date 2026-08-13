// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/EquippableFragment.h"
#include "Items/ItemActor.h"
#include "Core/WoodcraftTypes.h"

void UEquippableFragment::OnItemSpawned(AItemActor* ItemActor)
{	
    if (UStaticMeshComponent* Mesh = ItemActor->GetItemMesh())
    {
        Mesh->SetCollisionResponseToChannel(TRACE_EQUIPPABLE, ECollisionResponse::ECR_Overlap);
    }
}
// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/Fragments/EquippableFragment.h"

void UEquippableFragment::OnItemSpawned(AItemActor* ItemActor)
{	
    if (UStaticMeshComponent* Mesh = ItemActor->GetItemMesh())
    {
        Mesh->SetCollisionResponseToChannel(COLLISION_EQUIPPABLE, ECollisionResponse::ECR_Block);
    }
}
// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/AnimNotify_CompletePickup.h"
#include "Components/HeldItemsComponent.h"

void UAnimNotify_CompletePickup::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp) return;

    // Get the Actor owning the skeletal mesh (your Player Character)
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    UHeldItemsComponent* HeldItemsComponent = Owner->FindComponentByClass<UHeldItemsComponent>();
    if (!HeldItemsComponent) return;
    
    HeldItemsComponent->CompletePickup(Hand);
}
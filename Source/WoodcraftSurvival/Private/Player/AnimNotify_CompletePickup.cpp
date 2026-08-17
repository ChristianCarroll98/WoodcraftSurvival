// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/AnimNotify_CompletePickup.h"
#include "Components/HeldItemsComponent.h"

void UAnimNotify_CompletePickup::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
            TEXT("MeshComp Invalid in CompletePickup Notify for hand: ") + UEnum::GetValueAsString(Hand));
        return;
    }

    // Get the Actor owning the skeletal mesh (your Player Character)
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
            TEXT("Owner Actor Invalid in CompletePickup Notify for hand: ") + UEnum::GetValueAsString(Hand));
        return;
    }

    UHeldItemsComponent* HeldItemsComponent = Owner->FindComponentByClass<UHeldItemsComponent>();
    if (!HeldItemsComponent)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
            TEXT("Could not get HeldItemsComponent in CompletePickup Notify for hand: ")
            + UEnum::GetValueAsString(Hand));
        return;
    }
    
    HeldItemsComponent->CompletePickup(Hand);
}
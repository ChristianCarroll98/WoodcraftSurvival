// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/AnimNotify_CompletePickup.h"
#include "Components/HeldItemsComponent.h"

void UAnimNotify_CompletePickup::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    FString HandStr = Hand == EHand::Left ? "Left" : "Right";
    FString FullString = TEXT("UAnimNotify_CompletePickup::Notify called for hand: ") + HandStr;
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FullString);

    if (!MeshComp) return;

    // Get the Actor owning the skeletal mesh (your Player Character)
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    UHeldItemsComponent* HeldItemsComponent = Owner->FindComponentByClass<UHeldItemsComponent>();
    if (!HeldItemsComponent) return;
    
    HeldItemsComponent->CompletePickup(Hand);
}
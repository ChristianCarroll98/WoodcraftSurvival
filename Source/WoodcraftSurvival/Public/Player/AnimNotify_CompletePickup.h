// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "Core/WoodcraftTypes.h"
#include "AnimNotify_CompletePickup.generated.h"

UCLASS()
class WOODCRAFTSURVIVAL_API UAnimNotify_CompletePickup : public UAnimNotify
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    EHand Hand = EHand::Left;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

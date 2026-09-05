// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/AnimNotify_CraftIntroDone.h"
#include "Player/CraftingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_CraftIntroDone::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UCraftingComponent* Crafting = Owner->FindComponentByClass<UCraftingComponent>();
	if (!Crafting) return;

	float MontagePosition = 0.f;
	if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
	{
		if (UAnimMontage* PlayingMontage = AnimInstance->GetCurrentActiveMontage())
		{
			MontagePosition = AnimInstance->Montage_GetPosition(PlayingMontage);
		}
		else if (const UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
		{
			MontagePosition = AnimInstance->Montage_GetPosition(Montage);
		}
	}

	Crafting->NotifyCraftIntroDone(MontagePosition);
}

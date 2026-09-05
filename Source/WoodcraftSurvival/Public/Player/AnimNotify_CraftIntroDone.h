// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CraftIntroDone.generated.h"

/**
 * Place on the FPArms TwoHanded stage montage at the first interactive frame.
 * Reports montage time to UCraftingComponent. Presentations do not use this notify.
 */
UCLASS()
class WOODCRAFTSURVIVAL_API UAnimNotify_CraftIntroDone : public UAnimNotify
{
	GENERATED_BODY()

public:

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "Core/WoodcraftTypes.h"
#include "FPArmsAnimInstance.generated.h"

UCLASS()
class WOODCRAFTSURVIVAL_API UFPArmsAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	/** Neutral pose for the left arm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Held Items|Neutral")
	TObjectPtr<UAnimSequence> NeutralPoseLeft;

	/** Neutral pose for the right arm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Held Items|Neutral")
	TObjectPtr<UAnimSequence> NeutralPoseRight;

	/** Extended pose for the left arm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Held Items|Extended")
	TObjectPtr<UAnimSequence> ExtendedPoseLeft;

	/** Extended pose for the right arm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Held Items|Extended")
	TObjectPtr<UAnimSequence> ExtendedPoseRight;

	/** Extended state flag for the left arm */
	UPROPERTY(BlueprintReadWrite, Category = "Held Items|State")
	bool bLeftExtended;

	/** Extended state flag for the right arm */
	UPROPERTY(BlueprintReadWrite, Category = "Held Items|State")
	bool bRightExtended;

	/** Pushes a hard reference into the correct side. Safe to call from any thread context that already has the loaded sequence */
	UFUNCTION(BlueprintCallable, Category = "Held Items")
	void SetHoldPose(EHand Hand, UAnimSequence* NeutralPose, UAnimSequence* ExtendedPose);
};

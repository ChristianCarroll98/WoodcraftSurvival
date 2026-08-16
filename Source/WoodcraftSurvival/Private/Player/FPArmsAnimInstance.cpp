// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/FPArmsAnimInstance.h"

void UFPArmsAnimInstance::SetHoldPose(EHand Hand, UAnimSequence* NeutralPose, UAnimSequence* ExtendedPose)
{
	if (Hand == EHand::Left)
	{
		NeutralPoseLeft = NeutralPose;
		ExtendedPoseLeft = ExtendedPose;
	}
	else if (Hand == EHand::Right)
	{
		NeutralPoseRight = NeutralPose;
		ExtendedPoseRight = ExtendedPose;
	}
}

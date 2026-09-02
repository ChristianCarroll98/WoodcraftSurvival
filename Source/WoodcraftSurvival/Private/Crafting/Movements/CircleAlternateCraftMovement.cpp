// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Crafting/Movements/CircleAlternateCraftMovement.h"

UCircleAlternateCraftMovement::UCircleAlternateCraftMovement()
{
	MotionMode = ECraftingMotionMode::Guided;
	bWorkingLockX = true;
	bWorkingLockY = true;
	bWorkingLockZ = true;
	WorkingLinearStrength = 1000.f;
	WorkingAngularStrength = 1000.f;
}

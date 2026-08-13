#pragma once
// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WoodcraftTypes.generated.h"

UENUM(BlueprintType)
enum class EHand : uint8
{
	Left,
	Right,
	None
};



// ---------- custom collision channels ----------

/** The collision channel used for pickupable object detection. */
#define COLLISION_EQUIPPABLE ECollisionChannel::ECC_GameTraceChannel1

/** The collision channel used for player collision detection. */
#define COLLISION_PLAYER ECollisionChannel::ECC_GameTraceChannel2

/** The collision channel used for world collision detection. */
#define COLLISION_WORLD ECollisionChannel::ECC_GameTraceChannel3

/** The collision channel used for item collision detection. */
#define COLLISION_ITEM ECollisionChannel::ECC_GameTraceChannel4

/** The collision channel used for structure collision detection. */
//#define COLLISION_STRUCTURE ECollisionChannel::ECC_GameTraceChannel5

/** The collision channel used for creature collision detection. */
//#define COLLISION_CREATURE ECollisionChannel::ECC_GameTraceChannel6
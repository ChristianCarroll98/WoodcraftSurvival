// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "WoodcraftTypes.generated.h"


// ---------- Const Global Vars ----------

/** Error string prefix */
const FString GErrorPrefix = TEXT("ERROR: ");

/** Minimum impulse required for a collision to deal damage. Tunable later. */
const float GMinImpulse = 100.f;


// ---------- Enums ----------

/** Enum representing the player's two hands. */
UENUM(BlueprintType)
enum class EHand : uint8
{
	Left,
	Right,
	None
};


// ---------- Custom Collision Channels ----------

/** The collision channel used for player collision. */
#define COLLISION_PLAYER ECollisionChannel::ECC_GameTraceChannel2

/** The collision channel used for item collision. */
#define COLLISION_ITEM ECollisionChannel::ECC_GameTraceChannel4

/** The collision channel used for harvestable collision. */
#define COLLISION_HARVESTABLE ECollisionChannel::ECC_GameTraceChannel5


// ---------- Custom Trace Channels ----------

/** The collision channel used for equippable object detection. */
#define TRACE_EQUIPPABLE ECollisionChannel::ECC_GameTraceChannel1

/** The collision channel used for world/ground detection. */
#define TRACE_WORLD ECollisionChannel::ECC_GameTraceChannel3


/** The collision channel used for structure collision detection. */
//#define COLLISION_STRUCTURE ECollisionChannel::ECC_GameTraceChannel5  // now used by HARVESTABLE

/** The collision channel used for creature collision detection. */
//#define COLLISION_CREATURE ECollisionChannel::ECC_GameTraceChannel6

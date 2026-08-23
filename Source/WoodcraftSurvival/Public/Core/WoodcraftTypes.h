// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "WoodcraftTypes.generated.h"


// ---------- Const Global Vars ----------

/** Error string prefix */
const FString GErrorPrefix = TEXT("ERROR: ");

/** Minimum impulse required for a collision to deal damage. Tunable later. */
const float GMinImpulse = 100.f;

/** Minimum linear speed (cm/s) of the item mesh required to deal damage.
 *  Prevents continuous contact while pressed into a surface from repeatedly damaging. */
const float GMinItemSpeed = 80.f;

/** Max angle (degrees) between item velocity and preferred strike axis to keep Slash damage.
 *  Outside this cone the hit is forced to Blunt. */
const float GSlashMaxAngleDeg = 35.f;

/** Max angle (degrees) between item velocity and preferred strike axis to keep Pierce damage.
 *  Outside this cone the hit is forced to Blunt. */
const float GPierceMaxAngleDeg = 55.f;


// ---------- Enums ----------

/** Enum representing the player's two hands. */
UENUM(BlueprintType)
enum class EHand : uint8
{
	Left,
	Right,
	None
};

/**
 * Preferred strike axis / orientation mode for equippable items.
 * Used by incidence (angle → type conversion) and later by procedural swing orientation.
 * - None / Blunt: no dynamic rotation, Slash/Pierce candidates demoted to Blunt.
 * - SingleEdged: local +Y only.
 * - DoubleEdged: local ±Y (closer / Abs).
 * - Pierce: local +Z (tip).
 */
UENUM(BlueprintType)
enum class EItemStrikeMode : uint8
{
	None		UMETA(DisplayName = "None / Blunt"),
	SingleEdged	UMETA(DisplayName = "Single Edged (+Y)"),
	DoubleEdged	UMETA(DisplayName = "Double Edged (±Y)"),
	Pierce		UMETA(DisplayName = "Pierce (+Z)"),
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

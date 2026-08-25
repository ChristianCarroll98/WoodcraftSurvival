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

/** Min linear speed (cm/s) of a held item before procedural swing orientation activates.
 *  Relative to owner velocity. 80 after testing — 150 was too high for some swings. */
const float GMinSwingOrientSpeed = 80.f;

/** Angular strength multiplier (× MassScale) at low item speed while orienting. */
const float GOrientStrengthMin = 8.0f;

/** Angular strength multiplier (× MassScale) at high item speed while orienting. */
const float GOrientStrengthMax = 20.0f;

/** Raw item speed (cm/s) at which orient angular strength reaches GOrientStrengthMax. */
const float GOrientStrengthFullSpeed = 300.f;

/** Baseline angular strength multiplier (× MassScale) used on attach and when orient ends. */
const float GOrientStrengthBaseline = 5.5f;

/** Linear strength multiplier (× MassScale) at low item speed while orienting. */
const float GOrientLinearStrengthMin = 3.5f;

/** Linear strength multiplier (× MassScale) at high item speed while orienting. */
const float GOrientLinearStrengthMax = 8.0f;

/** Baseline linear strength multiplier (× MassScale) on attach and when orient ends. */
const float GOrientLinearStrengthBaseline = 2.8f;


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

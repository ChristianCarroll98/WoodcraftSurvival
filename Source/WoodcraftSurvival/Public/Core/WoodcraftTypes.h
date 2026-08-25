// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "WoodcraftTypes.generated.h"


// ---------- Const Global Vars ----------

/** Error string prefix */
const FString GErrorPrefix = TEXT("ERROR: ");

/** Master switch for hit / orientation on-screen hit text. */
const bool GbDebugPrint = true;

/** Master switch for hit / orientation debug draws. */
const bool GbDebugDraw = false;

/** Minimum impulse required for a collision to deal damage. Tunable later. */
const float GMinImpulse = 100.f;

/** Minimum linear speed (cm/s) of the item mesh required to deal damage and 
 *  before procedural swing orientation activates. Relative to owner velocity.
 *  Prevents continuous contact while pressed into a surface from repeatedly damaging. */
const float GMinItemSpeed = 0.f;

/** Accepted half-angle (degrees) from the blade plane for Slash.
 *  Velocity farther off the plane than this (more face-on) is forced to Blunt. */
const float GSlashMaxAngleFromPlaneDeg = 25.f;

/** Accepted half-angle (degrees) from preferred edge axis (+Y / ±Y) for Slash.
 *  Outside this → Blunt. Wide so lag / angled cuts still register. */
const float GSlashMaxAngleDeg = 75.f;

/** Accepted half-angle (degrees) from tip axis (+Z) for Pierce.
 *  Outside this → Blunt. */
const float GPierceMaxAngleDeg = 35.f;

/** Angular strength multiplier (× MassScale) at low item speed while orienting. */
const float GOrientStrengthMin = 5.0f;

/** Angular strength multiplier (× MassScale) at high item speed while orienting. */
const float GOrientStrengthMax = 10.0f;

/** Raw item speed (cm/s) at which orient angular strength reaches GOrientStrengthMax. */
const float GOrientStrengthFullSpeed = 300.f;

/** Baseline angular strength multiplier (× MassScale) used on attach and when orient ends. */
const float GOrientStrengthBaseline = 5.5f;

/** Linear strength multiplier (× MassScale) at low item speed while orienting. */
const float GOrientLinearStrengthMin = 2.5f;

/** Linear strength multiplier (× MassScale) at high item speed while orienting. */
const float GOrientLinearStrengthMax = 5.0f;

/** Baseline linear strength multiplier (× MassScale) on attach and when orient ends. */
const float GOrientLinearStrengthBaseline = 2.8f;

/** Max wrist twist clockwise from neutral (degrees). Positive Atan2 around WeaponBone +Z. */
const float GWristLimitCWDeg = 105.f;

/** Max wrist twist counter-clockwise from neutral (degrees). Negative Atan2 around WeaponBone +Z. */
const float GWristLimitCCWDeg = 135.f;

/** Extra degrees past the relevant limit before a preferred-edge flip is allowed. */
const float GWristHysteresisDeg = 12.f;


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

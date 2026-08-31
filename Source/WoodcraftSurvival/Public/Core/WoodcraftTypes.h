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

/** Minimum linear speed (cm/s) of the item mesh.
 *  Damage uses world speed of the hitting mesh (sprinting into something while extended can hit).
 *  Procedural orientation uses owner-relative primary-mesh velocity as its on/off gate.
 *  Prevents continuous contact while pressed into a surface from repeatedly damaging. */
const float GMinItemSpeed = 80.f;

/** Minimum look-delta rate (LookDelta.Size() / DeltaTime) to snap-orient.
 *  Tune from the cyan “look speed” print. Damage still uses GMinItemSpeed / GMinImpulse. */
const float GMinLookSpeed = 50.f;

/** Accepted half-angle (degrees) from the blade plane for Slash.
 *  Velocity farther off the plane than this (more face-on) is forced to Blunt. */
const float GSlashMaxAngleFromPlaneDeg = 25.f;

/** Accepted half-angle (degrees) from preferred edge axis (+Y / ±Y) for Slash.
 *  Outside this → Blunt. Wide so lag / angled cuts still register. */
const float GSlashMaxAngleDeg = 85.f;

/** Accepted half-angle (degrees) from tip axis (+Z) for Pierce.
 *  Outside this → Blunt. */
const float GPierceMaxAngleDeg = 35.f;

/** Look-delta rate (LookDelta.Size() / DeltaTime) at which swipe strength reaches max.
 *  Tune from the on-screen “look speed max2s” print. Not item cm/s. */
const float GControlStrengthFullLookSpeed = 240.f;

/** Exponent on the 0–1 look-speed factor before lerping swipe strength. 1 = linear, 2+ = ease-in. */
const float GControlStrengthCurveExp = 3.f;

/** Angular mass (kg) at which MassScale == 1 before the exponent. */
const float GControlMassRef = 0.6f;

/** Exponent on (mass / Ref) for angular MassScale. 1 = linear. */
const float GControlMassExp = 1.0f;

/** Floor on angular MassScale. */
const float GControlMassScaleMin = 0.35f;

/** Cap on angular MassScale so very heavy items do not over-stiffen rotation. */
const float GControlMassScaleMax = 6.0f;

/** Linear mass (kg) at which MassScaleLinear == 1 before the exponent. Pinned to the hatchet. */
const float GControlLinearMassRef = 1.67f;

/** Exponent on (mass / LinearRef) for MassScaleLinear. >1 keeps rocks/fists sloppy and climbs near tool mass. */
const float GControlLinearMassExp = 3.0f;

/** Floor on MassScaleLinear. */
const float GControlLinearMassScaleMin = 0.4f;

/** Cap on MassScaleLinear. */
const float GControlLinearMassScaleMax = 10.0f;

/** Angular strength multiplier (× MassScale) while not extended — planted by the body, slight give. */
const float GControlAngularStrengthNeutral = 6.f;

/** Angular strength multiplier (× MassScale) while extended with no / slow look. Combat lag floor. */
const float GControlAngularStrengthBaseline = 2.f;

/** Angular strength multiplier (× MassScale) at high look speed while extended. */
const float GControlAngularStrengthMax = 24.0f;

/** Linear strength multiplier (× MassScaleLinear) while not extended. */
const float GControlLinearStrengthNeutral = 8.f;

/** Linear strength multiplier (× MassScaleLinear) while extended with no / slow look. */
const float GControlLinearStrengthBaseline = 5.f;

/** Linear strength multiplier (× MassScaleLinear) at high look speed while extended. */
const float GControlLinearStrengthMax = 16.0f;

/** Max wrist twist clockwise from neutral (degrees).
 *  From the player's view: negative Atan2 around WeaponBone +Z. */
const float GWristLimitCWDeg = 105.f;

/** Max wrist twist counter-clockwise from neutral (degrees).
 *  From the player's view: positive Atan2 around WeaponBone +Z. */
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
 * Used by incidence (angle → type conversion) and procedural swing orientation.
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

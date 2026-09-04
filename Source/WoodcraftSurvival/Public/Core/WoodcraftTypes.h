// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "WoodcraftTypes.generated.h"


// ---------- Const Global Vars ----------

/** Error string prefix */
const FString GErrorPrefix = TEXT("ERROR: ");

/** Hit / incidence text and arrows. */
const bool GbDebugHits = true;

/** Swing speed, look speed, control strengths, wrist-limit draws. */
const bool GbDebugSwing = true;

/** Craft prompt and session text. Later minigame volume draws. */
const bool GbDebugCraft = true;

/** Harvestable health / yield / missing-mesh text. */
const bool GbDebugHarvest = true;

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
const float GSlashMaxAngleFromPlaneDeg = 30.f;

/** Accepted half-angle (degrees) from preferred edge axis (+Y / ±Y) for Slash.
 *  Outside this → Blunt. Wide so lag / angled cuts still register. */
const float GSlashMaxAngleDeg = 90.f;

/** Accepted half-angle (degrees) from tip axis (+Z) for Pierce.
 *  Outside this → Blunt. */
const float GPierceMaxAngleDeg = 35.f;

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

/** Role of one recipe slot. A2 recipes use Ingredient. */
UENUM(BlueprintType)
enum class ECraftingSlotRole : uint8
{
	Ingredient	UMETA(DisplayName = "Ingredient"),
	Tool		UMETA(DisplayName = "Tool"),
	Workpiece	UMETA(DisplayName = "Workpiece"),
};

/** How a recipe slot chooses a legal item. */
UENUM(BlueprintType)
enum class ECraftingSlotMatchMode : uint8
{
	ExactDefinition	UMETA(DisplayName = "Exact Definition"),
	RequiredTag		UMETA(DisplayName = "Required Tag"),
	Sharpenable		UMETA(DisplayName = "Sharpenable"),
};

/** How the session shows progress on the work. */
UENUM(BlueprintType)
enum class ECraftingAppearanceMode : uint8
{
	LiveMeshes		UMETA(DisplayName = "Live Meshes"),
	Morph			UMETA(DisplayName = "Morph"),
	SkeletalPhase	UMETA(DisplayName = "Skeletal Phase"),
};

/** How morph weights are sampled from progress. */
UENUM(BlueprintType)
enum class ECraftingMorphSampleMode : uint8
{
	Lerp		UMETA(DisplayName = "Lerp"),
	HoldAndPop	UMETA(DisplayName = "Hold And Pop"),
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

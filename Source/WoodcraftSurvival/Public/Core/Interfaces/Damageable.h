// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Damageable.generated.h"

/**
 * Minimal damage payload for the first pass.
 * Expand later with tool references, exact hit bone, etc.
 * Final damage calculation (tool effectiveness, hardness, etc.) can live
 * on the tool, a shared helper, or a fragment – the interface only provides
 * the common entry point.
 */
USTRUCT(BlueprintType)
struct FDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float Amount = 0.f;

	/** Simple tags or expand to FGameplayTagContainer later. */
	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	FName DamageType = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	TObjectPtr<AActor> Instigator = nullptr;

	// Optional: tool / item reference can be added when needed.
};

UINTERFACE(MinimalAPI, Blueprintable)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Shared interface implemented by both AItemActor and AHarvestableActor
 * so tools can damage either uniformly.
 */
class WOODCRAFTSURVIVAL_API IDamageable
{
	GENERATED_BODY()

public:
	virtual void ApplyDamage(const FDamageInfo& DamageInfo) = 0;
};

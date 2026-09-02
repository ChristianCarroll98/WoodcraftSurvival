// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WoodcraftCharacter.generated.h"

class UHeldItemsComponent;
class UCraftingComponent;

UCLASS()
class WOODCRAFTSURVIVAL_API AWoodcraftCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AWoodcraftCharacter();

	/** Owns holding logic, extend functionality, constraint activation, etc. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items|Holding")
	TObjectPtr<UHeldItemsComponent> HeldItemsComponent;

	/** Owns recipe table, snapshot, match prompt, and the craft session. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UCraftingComponent> CraftingComponent;
	
	// Future: interaction traces, input helpers, etc. can live here or on the component

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};

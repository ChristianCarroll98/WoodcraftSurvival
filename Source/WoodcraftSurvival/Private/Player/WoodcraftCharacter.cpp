// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/WoodcraftCharacter.h"
#include "Player/HeldItemsComponent.h"

// Sets default values
AWoodcraftCharacter::AWoodcraftCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HeldItemComponent = CreateDefaultSubobject<UHeldItemsComponent>(TEXT("HeldItemsComponent"));
}

// Called when the game starts or when spawned
void AWoodcraftCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWoodcraftCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AWoodcraftCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


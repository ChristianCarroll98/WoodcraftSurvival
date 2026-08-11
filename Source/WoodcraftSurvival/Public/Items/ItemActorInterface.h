// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemActorInterface.generated.h"

class UItemInstance;

UINTERFACE(MinimalAPI, Blueprintable)
class UItemActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for any actor that represents an item in the world.
 * Allows systems to query or interact with the item without knowing the concrete class.
 */
class WOODCRAFTSURVIVAL_API IItemActorInterface
{
	GENERATED_BODY()

public:

	/** Returns the ItemInstance this actor is currently representing. */
	virtual UItemInstance* GetItemInstance() const = 0;

	/** Initializes the actor from an existing ItemInstance (sets mesh, stores the instance, etc.). */
	virtual void InitializeFromInstance(UItemInstance* Instance) = 0;
};

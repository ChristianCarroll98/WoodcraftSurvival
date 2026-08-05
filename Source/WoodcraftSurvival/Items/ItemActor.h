#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemActorInterface.h"
#include "ItemActor.generated.h"

class UItemInstance;
class UStaticMeshComponent;

/**
 * World representation of an item.
 * Root component is always a StaticMeshComponent.
 * Holds a UItemInstance and implements IItemActorInterface.
 */
UCLASS()
class WOODCRAFTSURVIVAL_API AItemActor : public AActor, public IItemActorInterface
{
	GENERATED_BODY()

public:

	AItemActor();

	// ----- IItemActorInterface -----
	virtual UItemInstance* GetItemInstance() const override;
	virtual void InitializeFromInstance(UItemInstance* Instance) override;

protected:

	/** The runtime item data this actor represents. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemInstance> ItemInstance;

	/** Root mesh component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};

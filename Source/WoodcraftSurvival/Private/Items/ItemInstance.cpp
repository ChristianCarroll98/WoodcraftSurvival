// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Items/ItemInstance.h"
#include "Net/UnrealNetwork.h"

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemInstance, ItemDefinition);
	DOREPLIFETIME(UItemInstance, UniqueId);
	DOREPLIFETIME(UItemInstance, CurrentHealth);
}

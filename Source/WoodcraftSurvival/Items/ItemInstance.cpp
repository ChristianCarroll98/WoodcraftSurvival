#include "ItemInstance.h"
#include "Net/UnrealNetwork.h"

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemInstance, Definition);
	DOREPLIFETIME(UItemInstance, UniqueId);
	DOREPLIFETIME(UItemInstance, CurrentHealth);
}

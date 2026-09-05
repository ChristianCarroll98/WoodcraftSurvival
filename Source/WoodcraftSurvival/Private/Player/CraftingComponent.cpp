// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/CraftingComponent.h"
#include "Items/ItemActor.h"
#include "Items/ItemInstance.h"
#include "Items/ItemFactorySubsystem.h"
#include "Items/Fragments/DurabilityItemFragment.h"
#include "Player/HeldItemsComponent.h"
#include "Core/WoodcraftTypes.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Components/SceneComponent.h"

namespace
{
	constexpr int32 CraftPromptMessageId = 8201;
}

UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveRecipeAssets();

	HeldItems = GetOwner() ? GetOwner()->FindComponentByClass<UHeldItemsComponent>() : nullptr;
	if (HeldItems)
	{
		HeldItems->OnHeldItemsChanged.AddUObject(this, &UCraftingComponent::HandleHeldItemsChanged);
	}

	HandleHeldItemsChanged();
}

void UCraftingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsSessionActive())
	{
		EndSession();
	}

	if (HeldItems)
	{
		HeldItems->OnHeldItemsChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool UCraftingComponent::IsSessionActive() const
{
	return Session.Recipe != nullptr;
}

EHand UCraftingComponent::GetEngageHand() const
{
	return IsSessionActive() ? Session.EngageHand : EHand::None;
}

bool UCraftingComponent::CanStartCraft() const
{
	if (IsSessionActive()) return false;
	if (CurrentSnapshot.bLeftExtended || CurrentSnapshot.bRightExtended) return false;
	return CurrentMatches.IsValidIndex(SelectedMatchIndex);
}

bool UCraftingComponent::TryStartCraft()
{
	if (!CanStartCraft()) return false;

	const FCraftingMatch& Match = CurrentMatches[SelectedMatchIndex];
	if (!Match.Recipe) return false;

	Session.Recipe = Match.Recipe;
	Session.Bindings = Match.Bindings;
	Session.Progress = 0.f;
	Session.Phase = 0;
	Session.bEngage = false;
	Session.EngageHand = ResolveEngageHand(Match);
	Session.Presentation = nullptr;

	PushCraftingIMC();
	ApplyGroundCraftView();
	UpdateDebugPrompt();
	return true;
}

void UCraftingComponent::CancelCraft()
{
	if (!IsSessionActive()) return;
	EndSession();
}

void UCraftingComponent::NotifyOwnerDamaged()
{
	if (!IsSessionActive()) return;
	EndSession();
}

void UCraftingComponent::CycleCraftMatch(int32 Delta)
{
	if (IsSessionActive()) return;
	if (CurrentMatches.Num() < 2) return;
	if (Delta == 0) return;

	const int32 Num = CurrentMatches.Num();
	SelectedMatchIndex = (SelectedMatchIndex + Delta) % Num;
	if (SelectedMatchIndex < 0) SelectedMatchIndex += Num;
	UpdateDebugPrompt();
}

void UCraftingComponent::SetCraftEngage(EHand Hand, bool bPressed)
{
	if (!IsSessionActive()) return;
	if (Hand == EHand::None) return;
	if (Hand != Session.EngageHand) return;

	Session.bEngage = bPressed;
	if (bPressed && bInstantCommitOnEngage)
	{
		CompleteCraft();
	}
}

void UCraftingComponent::HandleHeldItemsChanged()
{
	if (IsSessionActive()) return;

	if (LoadedRecipes.Num() == 0 && RecipeAssets.Num() > 0)
	{
		ResolveRecipeAssets();
	}

	RebuildSnapshot();
	RefreshMatches();
	UpdateDebugPrompt();
}

void UCraftingComponent::ResolveRecipeAssets()
{
	LoadedRecipes.Reset();
	for (const TSoftObjectPtr<UCraftingRecipeDefinition>& RecipeAsset : RecipeAssets)
	{
		if (UCraftingRecipeDefinition* Recipe = RecipeAsset.LoadSynchronous())
		{
			LoadedRecipes.Add(Recipe);
		}
	}
}

void UCraftingComponent::FillHandSnapshot(EHand Hand)
{
	AItemActor* ItemActor = HeldItems ? HeldItems->GetHeldItem(Hand) : nullptr;
	const bool bUnarmed = HeldItems && HeldItems->GetIsUnarmed(Hand);
	const bool bExtended = HeldItems && HeldItems->GetIsExtended(Hand);

	if (Hand == EHand::Left)
	{
		CurrentSnapshot.LeftActor = ItemActor;
		CurrentSnapshot.bLeftUnarmed = bUnarmed;
		CurrentSnapshot.bLeftExtended = bExtended;
		CurrentSnapshot.bLeftStacked = HeldItems && HeldItems->IsHandStacked(Hand);
	}
	else if (Hand == EHand::Right)
	{
		CurrentSnapshot.RightActor = ItemActor;
		CurrentSnapshot.bRightUnarmed = bUnarmed;
		CurrentSnapshot.bRightExtended = bExtended;
		CurrentSnapshot.bRightStacked = HeldItems && HeldItems->IsHandStacked(Hand);
	}
}

void UCraftingComponent::RebuildSnapshot()
{
	FillHandSnapshot(EHand::Left);
	FillHandSnapshot(EHand::Right);

	CurrentSnapshot.Station.Reset();
	CurrentSnapshot.StationActor = nullptr;
}

void UCraftingComponent::RefreshMatches()
{
	TArray<UCraftingRecipeDefinition*> Recipes;
	Recipes.Reserve(LoadedRecipes.Num());
	for (UCraftingRecipeDefinition* Recipe : LoadedRecipes)
	{
		if (Recipe) Recipes.Add(Recipe);
	}

	CurrentMatches = UCraftingRecipeDefinition::FindMatches(CurrentSnapshot, Recipes);

	if (CurrentMatches.Num() == 0)
	{
		SelectedMatchIndex = 0;
		return;
	}

	if (!CurrentMatches.IsValidIndex(SelectedMatchIndex))
	{
		SelectedMatchIndex = 0;
	}
}

void UCraftingComponent::UpdateDebugPrompt() const
{
	if (!GEngine) return;

	if (!GbDebugCraft)
	{
		GEngine->AddOnScreenDebugMessage(CraftPromptMessageId, 0.f, FColor::Cyan, FString());
		return;
	}

	if (IsSessionActive())
	{
		GEngine->AddOnScreenDebugMessage(
			CraftPromptMessageId,
			10000.f,
			FColor::Cyan,
			TEXT("[R] Cancel"));
		return;
	}

	const bool bBothNeutral = !CurrentSnapshot.bLeftExtended && !CurrentSnapshot.bRightExtended;
	const bool bShowPrompt = bBothNeutral && CurrentMatches.IsValidIndex(SelectedMatchIndex);
	if (!bShowPrompt)
	{
		GEngine->AddOnScreenDebugMessage(CraftPromptMessageId, 0.f, FColor::Cyan, FString());
		return;
	}

	const FCraftingMatch& Match = CurrentMatches[SelectedMatchIndex];
	const FText DisplayName = Match.Recipe ? Match.Recipe->DisplayName : FText();
	const FString CraftName = DisplayName.IsEmpty()
		? (Match.Recipe ? Match.Recipe->GetName() : TEXT("?"))
		: DisplayName.ToString();

	GEngine->AddOnScreenDebugMessage(
		CraftPromptMessageId,
		10000.f,
		FColor::Cyan,
		FString::Printf(TEXT("[E] Craft %s"), *CraftName));
}

EHand UCraftingComponent::ResolveEngageHand(const FCraftingMatch& Match) const
{
	if (Match.Recipe)
	{
		for (const FCraftingSlotBinding& Binding : Match.Bindings)
		{
			if (Binding.bStation) continue;
			if (Binding.Hand == EHand::None) continue;
			if (!Match.Recipe->Slots.IsValidIndex(Binding.SlotIndex)) continue;
			if (Match.Recipe->Slots[Binding.SlotIndex].Role != ECraftingSlotRole::Tool) continue;
			return Binding.Hand;
		}
	}

	return DefaultCraftHand;
}

EHand UCraftingComponent::ResolveAutoEquipHand() const
{
	if (Session.Recipe)
	{
		for (const FCraftingSlotBinding& Binding : Session.Bindings)
		{
			if (Binding.bStation) continue;
			if (Binding.Hand == EHand::None) continue;
			if (!Session.Recipe->Slots.IsValidIndex(Binding.SlotIndex)) continue;
			if (Session.Recipe->Slots[Binding.SlotIndex].Role != ECraftingSlotRole::Tool) continue;
			return (Binding.Hand == EHand::Left) ? EHand::Right : EHand::Left;
		}
	}

	return DefaultCraftHand;
}

void UCraftingComponent::CompleteCraft()
{
	if (!IsSessionActive() || !Session.Recipe) return;
	if (!HeldItems) return;

	UItemFactorySubsystem* ItemFactory = GetWorld()
		? GetWorld()->GetSubsystem<UItemFactorySubsystem>()
		: nullptr;
	if (!ItemFactory) return;

	const UCraftingRecipeDefinition* Recipe = Session.Recipe;

	for (const FCraftingSlotBinding& Binding : Session.Bindings)
	{
		if (!Recipe->Slots.IsValidIndex(Binding.SlotIndex)) continue;
		const FCraftingSlot& Slot = Recipe->Slots[Binding.SlotIndex];
		if (Slot.DurabilityCost <= 0.f) continue;
		if (!Binding.Actor) continue;

		UItemInstance* Instance = Binding.Actor->GetItemInstance();
		if (!Instance) continue;
		if (!Instance->FindFragment<UDurabilityItemFragment>()) continue;

		Instance->CurrentHealth = FMath::Max(0.f, Instance->CurrentHealth - Slot.DurabilityCost);
	}

	for (const FCraftingSlotBinding& Binding : Session.Bindings)
	{
		if (!Recipe->Slots.IsValidIndex(Binding.SlotIndex)) continue;
		if (!Recipe->Slots[Binding.SlotIndex].bConsumed) continue;
		if (Binding.bStation) continue;
		if (Binding.Hand == EHand::None) continue;
		HeldItems->DestroyHeldItem(Binding.Hand);
	}

	const EHand AutoEquipHand = ResolveAutoEquipHand();
	bool bDidAutoEquip = false;
	int32 PileIndex = 0;

	if (GbDebugCraft && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			6.f,
			FColor::Cyan,
			FString::Printf(
				TEXT("CompleteCraft %s outputs=%d"),
				*Recipe->GetName(),
				Recipe->Outputs.Num()));
	}

	for (const FCraftingOutput& Output : Recipe->Outputs)
	{
		if (!Output.ItemDefinition)
		{
			if (GbDebugCraft && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Red,
					TEXT("CompleteCraft: output ItemDefinition is null"));
			}
			continue;
		}

		FTransform SpawnTransform = HeldItems->GetDropTransform();
		if (PileIndex > 0)
		{
			const FVector Offset(
				FMath::FRandRange(-12.f, 12.f),
				FMath::FRandRange(-12.f, 12.f),
				0.f);
			SpawnTransform.AddToTranslation(Offset);
		}

		AItemActor* Spawned = ItemFactory->SpawnItemActorFromDefinition(
			Output.ItemDefinition.Get(),
			SpawnTransform);
		if (!Spawned)
		{
			if (GbDebugCraft && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					6.f,
					FColor::Red,
					FString::Printf(TEXT("CompleteCraft: spawn failed for %s"),
						*GetNameSafe(Output.ItemDefinition.Get())));
			}
			continue;
		}

		const bool bTryEquip = Output.bAutoEquip && !bDidAutoEquip
			&& HeldItems->GetIsUnarmed(AutoEquipHand);
		if (bTryEquip && HeldItems->ReplaceHeldItem(AutoEquipHand, Spawned))
		{
			bDidAutoEquip = true;
			if (GbDebugCraft && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					6.f,
					FColor::Green,
					FString::Printf(TEXT("CompleteCraft: equipped %s"),
						*GetNameSafe(Spawned)));
			}
			continue;
		}

		++PileIndex;
		if (GbDebugCraft && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				6.f,
				FColor::Yellow,
				FString::Printf(TEXT("CompleteCraft: piled %s"),
					*GetNameSafe(Spawned)));
		}
	}

	EndSession();
}

void UCraftingComponent::EndSession()
{
	RestoreGroundCraftView();
	Session = FCraftingSession();
	PopCraftingIMC();

	RebuildSnapshot();
	RefreshMatches();
	UpdateDebugPrompt();
}

USceneComponent* UCraftingComponent::FindFirstPersonCamera() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	TArray<USceneComponent*> Components;
	OwnerActor->GetComponents<USceneComponent>(Components);
	for (USceneComponent* Component : Components)
	{
		if (Component && Component->GetName().StartsWith(TEXT("FPCamera")))
		{
			return Component;
		}
	}

	return nullptr;
}

void UCraftingComponent::ApplyGroundCraftView()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	USceneComponent* Camera = FindFirstPersonCamera();
	if (!PC && !Camera) return;

	if (!bCraftViewApplied)
	{
		if (PC) CachedControlRotation = PC->GetControlRotation();
		if (Camera) CachedCameraRelativeTransform = Camera->GetRelativeTransform();
		bCraftViewApplied = true;
	}

	if (PC)
	{
		FRotator ControlRotation = CachedControlRotation;
		ControlRotation.Pitch = 0.f;
		ControlRotation.Roll = 0.f;
		PC->SetControlRotation(ControlRotation);
	}

	if (Camera)
	{
		FTransform CraftTransform = GroundCraftCameraTransform;
		CraftTransform.SetScale3D(FVector::OneVector);
		Camera->SetRelativeTransform(CraftTransform);
	}
}

void UCraftingComponent::RestoreGroundCraftView()
{
	if (!bCraftViewApplied) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PC)
	{
		PC->SetControlRotation(CachedControlRotation);
	}

	if (USceneComponent* Camera = FindFirstPersonCamera())
	{
		Camera->SetRelativeTransform(CachedCameraRelativeTransform);
	}

	bCraftViewApplied = false;
}

void UCraftingComponent::PushCraftingIMC()
{
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem();
	if (!InputSubsystem) return;

	if (GameplayMappingContext)
	{
		InputSubsystem->RemoveMappingContext(GameplayMappingContext);
	}

	if (CraftingMappingContext)
	{
		InputSubsystem->AddMappingContext(CraftingMappingContext, CraftingIMCPriority);
	}

	bCraftingIMCPushed = true;
}

void UCraftingComponent::PopCraftingIMC()
{
	if (!bCraftingIMCPushed) return;

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem();
	if (InputSubsystem)
	{
		if (CraftingMappingContext)
		{
			InputSubsystem->RemoveMappingContext(CraftingMappingContext);
		}

		if (GameplayMappingContext)
		{
			InputSubsystem->AddMappingContext(GameplayMappingContext, GameplayIMCPriority);
		}
	}

	bCraftingIMCPushed = false;
}

UEnhancedInputLocalPlayerSubsystem* UCraftingComponent::GetInputSubsystem() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC) return nullptr;

	const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer) return nullptr;

	return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}

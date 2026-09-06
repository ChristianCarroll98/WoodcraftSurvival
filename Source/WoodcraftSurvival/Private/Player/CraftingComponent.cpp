// Copyright (c) 2026 Christian Carroll. All Rights Reserved.

#include "Player/CraftingComponent.h"
#include "Crafting/Movements/CraftMovement.h"
#include "Crafting/Movements/GrindActiveCraftMovement.h"
#include "Items/ItemActor.h"
#include "Items/ItemInstance.h"
#include "Items/ItemFactorySubsystem.h"
#include "Items/Fragments/DurabilityItemFragment.h"
#include "Player/HeldItemsComponent.h"
#include "Player/FPArmsAnimInstance.h"
#include "Core/WoodcraftTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Components/SceneComponent.h"
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
	PrimaryComponentTick.bCanEverTick = true;
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

void UCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsSessionActive()) return;

	TickStageClock();
	if (!IsSessionActive()) return;
	if (Session.bIntroActive) return;

	TickGrindActive(DeltaTime);
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
	if (!Match.Recipe || Match.Recipe->GetStageCount() < 1) return false;

	Session.Recipe = Match.Recipe;
	Session.Bindings = Match.Bindings;
	Session.bEngage = false;
	Session.EngageHand = ResolveEngageHand(Match);
	Session.Presentation = nullptr;
	ApplyStage(0);

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

void UCraftingComponent::BindInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput) return;
	if (!CraftPointerAction) return;

	EnhancedInput->BindAction(
		CraftPointerAction,
		ETriggerEvent::Triggered,
		this,
		&UCraftingComponent::HandleCraftPointer);
	EnhancedInput->BindAction(
		CraftPointerAction,
		ETriggerEvent::Completed,
		this,
		&UCraftingComponent::HandleCraftPointerCompleted);
}

void UCraftingComponent::HandleCraftPointer(const FInputActionValue& Value)
{
	if (!IsSessionActive())
	{
		CraftPointer = FVector2D::ZeroVector;
		return;
	}

	CraftPointer = Value.Get<FVector2D>();
}

void UCraftingComponent::HandleCraftPointerCompleted()
{
	CraftPointer = FVector2D::ZeroVector;
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
	if (bPressed && bInstantComplete)
	{
		CompleteCurrentStage();
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
		const FCraftStage* Stage = Session.Recipe ? Session.Recipe->GetStage(Session.Phase) : nullptr;
		const UCraftMovement* Move = Stage ? Stage->Move.Get() : nullptr;
		const int32 StageCount = Session.Recipe ? Session.Recipe->GetStageCount() : 0;
		GEngine->AddOnScreenDebugMessage(
			CraftPromptMessageId,
			10000.f,
			FColor::Cyan,
			FString::Printf(
				TEXT("[R] Cancel  Stage %d/%d  %s  Intro=%s  P=%.2f"),
				Session.Phase + 1,
				StageCount,
				Move ? *Move->GetClass()->GetName() : TEXT("None"),
				Session.bIntroActive ? TEXT("1") : TEXT("0"),
				Session.Progress));
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

void UCraftingComponent::ApplyStage(int32 StageIndex)
{
	if (!Session.Recipe) return;

	const FCraftStage* Stage = Session.Recipe->GetStage(StageIndex);
	if (!Stage) return;

	EndCraftMotionBothHands();
	StopStageMontage();

	Session.Phase = StageIndex;
	Session.Progress = 0.f;
	Session.AccumulatedWork = 0.f;
	Session.bMotionStarted = false;
	Session.IntroEndTime = 0.f;
	Session.bIntroActive = !Stage->Montage.IsNull();

	ApplyStagePresentation(*Stage);
	PlayStageMontage(*Stage);

	if (GbDebugCraft && GEngine)
	{
		const UCraftMovement* Move = Stage->Move.Get();
		GEngine->AddOnScreenDebugMessage(
			-1,
			6.f,
			FColor::Cyan,
			FString::Printf(
				TEXT("Craft stage %d/%d %s DriveArms=%s Intro=%s"),
				StageIndex + 1,
				Session.Recipe->GetStageCount(),
				Move ? *Move->GetClass()->GetName() : TEXT("None"),
				(Move && Move->bProgressDrivesMontage) ? TEXT("1") : TEXT("0"),
				Session.bIntroActive ? TEXT("1") : TEXT("0")));
	}
}

void UCraftingComponent::NotifyCraftIntroDone(float MontagePosition)
{
	if (!IsSessionActive()) return;
	if (!Session.bIntroActive) return;

	Session.IntroEndTime = MontagePosition;
	Session.bIntroActive = false;

	const FCraftStage* Stage = Session.Recipe ? Session.Recipe->GetStage(Session.Phase) : nullptr;
	const UCraftMovement* Move = Stage ? Stage->Move.Get() : nullptr;
	if (HeldItems)
	{
		if (UFPArmsAnimInstance* ArmsAnim = HeldItems->GetArmsAnimInstance())
		{
			if (Session.PlayingMontage && (!Move || !Move->bProgressDrivesMontage))
			{
				ArmsAnim->Montage_SetPlayRate(Session.PlayingMontage, 0.f);
			}
		}
	}

	if (GbDebugCraft && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			6.f,
			FColor::Cyan,
			FString::Printf(TEXT("Craft intro done t=%.3f"), MontagePosition));
	}

	UpdateDebugPrompt();
}

void UCraftingComponent::CompleteCurrentStage()
{
	if (!IsSessionActive() || !Session.Recipe) return;

	const int32 LastIndex = Session.Recipe->GetStageCount() - 1;
	if (LastIndex < 0 || Session.Phase >= LastIndex)
	{
		CompleteCraft();
		return;
	}

	ApplyStage(Session.Phase + 1);
	UpdateDebugPrompt();
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

		const bool bTryEquip = Output.bAutoEquip && !bDidAutoEquip
			&& HeldItems->GetIsUnarmed(AutoEquipHand);

		FTransform SpawnTransform = bTryEquip
			? HeldItems->GetHeldSpawnTransform(AutoEquipHand)
			: HeldItems->GetDropTransform();
		if (!bTryEquip && PileIndex > 0)
		{
			const FVector Offset(
				FMath::FRandRange(-12.f, 12.f),
				FMath::FRandRange(-12.f, 12.f),
				0.f);
			SpawnTransform.AddToTranslation(Offset);
		}

		AItemActor* Spawned = ItemFactory->SpawnItemActorFromDefinition(
			Output.ItemDefinition.Get(),
			SpawnTransform,
			!bTryEquip);
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
	EndCraftMotionBothHands();
	StopStageMontage();
	RestoreBoundItemVisibility();
	RestoreGroundCraftView();
	CraftPointer = FVector2D::ZeroVector;
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

void UCraftingComponent::PlayStageMontage(const FCraftStage& Stage)
{
	StopStageMontage();
	if (Stage.Montage.IsNull())
	{
		Session.bIntroActive = false;
		return;
	}

	UAnimMontage* Montage = Stage.Montage.LoadSynchronous();
	if (!Montage)
	{
		Session.bIntroActive = false;
		return;
	}

	UFPArmsAnimInstance* ArmsAnim = HeldItems ? HeldItems->GetArmsAnimInstance() : nullptr;
	if (!ArmsAnim)
	{
		Session.bIntroActive = false;
		return;
	}

	ArmsAnim->Montage_Play(Montage);
	Session.PlayingMontage = Montage;
}

void UCraftingComponent::StopStageMontage()
{
	if (HeldItems)
	{
		if (UFPArmsAnimInstance* ArmsAnim = HeldItems->GetArmsAnimInstance())
		{
			if (Session.PlayingMontage)
			{
				ArmsAnim->Montage_Stop(0.15f, Session.PlayingMontage);
			}
		}
	}

	Session.PlayingMontage = nullptr;
}

void UCraftingComponent::TickStageClock()
{
	if (!HeldItems) return;

	UFPArmsAnimInstance* ArmsAnim = HeldItems->GetArmsAnimInstance();
	if (!ArmsAnim) return;

	if (Session.bIntroActive && Session.PlayingMontage)
	{
		if (!ArmsAnim->Montage_IsPlaying(Session.PlayingMontage))
		{
			NotifyCraftIntroDone(Session.PlayingMontage->GetPlayLength());
		}
		return;
	}

	const FCraftStage* Stage = Session.Recipe ? Session.Recipe->GetStage(Session.Phase) : nullptr;
	const UCraftMovement* Move = Stage ? Stage->Move.Get() : nullptr;
	if (!Move || !Move->bProgressDrivesMontage) return;
	if (!Session.PlayingMontage) return;

	const float Length = Session.PlayingMontage->GetPlayLength();
	const float T = FMath::Lerp(Session.IntroEndTime, Length, Session.Progress);
	ArmsAnim->Montage_SetPosition(Session.PlayingMontage, T);
	ArmsAnim->Montage_SetPlayRate(Session.PlayingMontage, 0.f);
}

void UCraftingComponent::ApplyStagePresentation(const FCraftStage& Stage)
{
	RestoreBoundItemVisibility();
	SetItemRenderHidden(GetBoundActor(EHand::Left), Stage.bHideLeft);
	SetItemRenderHidden(GetBoundActor(EHand::Right), Stage.bHideRight);
}

void UCraftingComponent::RestoreBoundItemVisibility()
{
	SetItemRenderHidden(GetBoundActor(EHand::Left), false);
	SetItemRenderHidden(GetBoundActor(EHand::Right), false);
}

void UCraftingComponent::SetItemRenderHidden(AItemActor* Item, bool bHidden) const
{
	if (!Item) return;

	if (UStaticMeshComponent* Primary = Item->GetItemPrimaryMesh())
	{
		Primary->SetHiddenInGame(bHidden);
	}
	if (UStaticMeshComponent* Secondary = Item->GetItemSecondaryMesh())
	{
		Secondary->SetHiddenInGame(bHidden);
	}
}

void UCraftingComponent::EndCraftMotionBothHands()
{
	if (!HeldItems) return;
	HeldItems->EndCraftMotion(EHand::Left);
	HeldItems->EndCraftMotion(EHand::Right);
}

EHand UCraftingComponent::GetSecondaryHand() const
{
	return (Session.EngageHand == EHand::Left) ? EHand::Right : EHand::Left;
}

AItemActor* UCraftingComponent::GetBoundActor(EHand Hand) const
{
	for (const FCraftingSlotBinding& Binding : Session.Bindings)
	{
		if (Binding.bStation) continue;
		if (Binding.Hand == Hand) return Binding.Actor;
	}
	return HeldItems ? HeldItems->GetHeldItem(Hand) : nullptr;
}

void UCraftingComponent::TickGrindActive(float DeltaTime)
{
	if (!HeldItems || !Session.Recipe) return;

	const UGrindActiveCraftMovement* Grind =
		Session.Recipe->FindMove<UGrindActiveCraftMovement>(Session.Phase);
	if (!Grind) return;

	const FCraftStage* Stage = Session.Recipe->GetStage(Session.Phase);
	if (!Stage) return;

	const EHand WorkingHand = Session.EngageHand;
	const EHand PlantedHand = GetSecondaryHand();
	if (WorkingHand == EHand::None || PlantedHand == EHand::None) return;

	if (!Session.bMotionStarted)
	{
		HeldItems->BeginCraftMotion(PlantedHand, Grind->PlantedLinearStrength, Grind->PlantedAngularStrength);
		HeldItems->SetCraftAxisLocks(PlantedHand, true, true, true);
		HeldItems->BeginCraftMotion(WorkingHand, Grind->WorkingLinearStrength, Grind->WorkingAngularStrength);
		HeldItems->SetCraftAxisLocks(WorkingHand, false, false, true);
		Session.bMotionStarted = true;
	}

	const FVector2D Pointer = CraftPointer;
	CraftPointer = FVector2D::ZeroVector;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const FVector WorldRight = OwnerPawn ? OwnerPawn->GetActorRightVector() : FVector::RightVector;
	const FVector WorldForward = OwnerPawn ? OwnerPawn->GetActorForwardVector() : FVector::ForwardVector;
	const FVector WorldDelta =
		(WorldRight * Pointer.X + WorldForward * Pointer.Y) * Grind->PointerSensitivity;

	const FTransform BoneXform = HeldItems->GetHeldSpawnTransform(WorkingHand);
	FVector Extra = HeldItems->GetCraftExtraOffset(WorkingHand);
	Extra += BoneXform.InverseTransformVector(WorldDelta);
	Extra.X = FMath::Clamp(Extra.X, -Grind->WorkingVolumeHalfExtents.X, Grind->WorkingVolumeHalfExtents.X);
	Extra.Y = FMath::Clamp(Extra.Y, -Grind->WorkingVolumeHalfExtents.Y, Grind->WorkingVolumeHalfExtents.Y);
	Extra.Z = 0.f;
	HeldItems->SetCraftExtraOffset(WorkingHand, Extra);

	AItemActor* WorkingItem = GetBoundActor(WorkingHand);
	float PlanarSpeed = 0.f;
	if (WorkingItem)
	{
		if (UStaticMeshComponent* Mesh = WorkingItem->GetItemPrimaryMesh())
		{
			const FVector Vel = Mesh->GetPhysicsLinearVelocity();
			PlanarSpeed = FVector(Vel.X, Vel.Y, 0.f).Size();
		}
	}

	if (PlanarSpeed >= Grind->MinStrokeSpeed)
	{
		const float Rate = FMath::Min(PlanarSpeed, Grind->MaxStrokeSpeed) / FMath::Max(Grind->MaxStrokeSpeed, 1.f);
		Session.AccumulatedWork += Rate * DeltaTime;
	}

	const float Required = FMath::Max(Stage->WorkRequired, 0.01f);
	Session.Progress = FMath::Clamp(Session.AccumulatedWork / Required, 0.f, 1.f);
	UpdateDebugPrompt();

	if (GbDebugCraft)
	{
		if (UWorld* World = GetWorld())
		{
			const FVector Origin = BoneXform.GetLocation();
			DrawDebugBox(
				World,
				Origin,
				Grind->WorkingVolumeHalfExtents,
				BoneXform.GetRotation(),
				FColor::Cyan,
				false,
				0.f,
				0,
				1.f);
		}
	}

	if (Session.Progress >= 1.f)
	{
		CompleteCurrentStage();
	}
}

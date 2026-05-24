// Copyright Epic Games, Inc. All Rights Reserved.


#include "CocinaSimulatorPlayerController.h"
#include "CocinaSimulator.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "EndMatchMenuWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ACocinaSimulatorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogCocinaSimulator, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void ACocinaSimulatorPlayerController::ShowEndMatchMenu(EEndMatchReason Reason, int32 FinalScore)
{
	if (EndMatchMenuWidget)
	{
		EndMatchMenuWidget->RemoveFromParent();
		EndMatchMenuWidget = nullptr;
	}

	if (EndMatchMenuWidgetClass)
	{
		EndMatchMenuWidget = CreateWidget<UEndMatchMenuWidget>(this, EndMatchMenuWidgetClass);
		if (EndMatchMenuWidget)
		{
			EndMatchMenuWidget->AddToPlayerScreen(1000);
			EndMatchMenuWidget->SetEndMatchSummary(Reason, FinalScore);
		}
	}
	else
	{
		BP_ShowEndMatchSummary(Reason, FinalScore);
	}

	if (IsLocalController())
	{
		FInputModeUIOnly InputMode;
		if (EndMatchMenuWidget)
		{
			InputMode.SetWidgetToFocus(EndMatchMenuWidget->TakeWidget());
		}
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void ACocinaSimulatorPlayerController::Client_ShowEndMatchMenu_Implementation(EEndMatchReason Reason, int32 FinalScore)
{
	ShowEndMatchMenu(Reason, FinalScore);
}

void ACocinaSimulatorPlayerController::ReturnToLobby()
{
	if (IsLocalController())
	{
		ClientReturnToMainMenuWithTextReason(FText::FromString(TEXT("Volviendo al lobby")));
	}
}

void ACocinaSimulatorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

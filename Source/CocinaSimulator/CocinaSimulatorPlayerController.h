// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CocinaSimulator.h"
#include "CocinaSimulatorPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UEndMatchMenuWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ACocinaSimulatorPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** End match summary menu widget class */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UEndMatchMenuWidget> EndMatchMenuWidgetClass;

	/** Pointer to the end match menu widget */
	TObjectPtr<UEndMatchMenuWidget> EndMatchMenuWidget;

public:
	UFUNCTION(Client, Reliable)
	void Client_ShowEndMatchMenu(EEndMatchReason Reason, int32 FinalScore);

	UFUNCTION(BlueprintCallable, Category="UI")
	void ShowEndMatchMenu(EEndMatchReason Reason, int32 FinalScore);

	UFUNCTION(BlueprintCallable, Category="UI")
	void ReturnToLobby();

	UFUNCTION(BlueprintImplementableEvent, Category="UI")
	void BP_ShowEndMatchSummary(EEndMatchReason Reason, int32 FinalScore);

protected:
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CocinaSimulator.h"
#include "GameFramework/GameModeBase.h"
#include "CocinaSimulatorGameMode.generated.h"

class APickUp;
class ACocinaSimulatorGameState;
class ACocinaSimulatorPlayerState;
class UUserWidget;

UCLASS()
class ACocinaSimulatorGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ACocinaSimulatorGameMode();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Delivery")
    void AddDelivery(APickUp* Item);

    UFUNCTION(BlueprintCallable, Category = "Delivery")
    int32 GetRemainingDeliveries() const;

    UFUNCTION(BlueprintCallable, Category = "Match")
    void CompleteMatch(EEndMatchReason Reason);

    UFUNCTION(BlueprintCallable, Category = "Match")
    void ShowEndMatchForPlayers(EEndMatchReason Reason);

    UPROPERTY(BlueprintReadOnly, Category = "Delivery")
    int32 DeliveriesCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Delivery")
    int32 RequiredDeliveries = 3;

    UPROPERTY(BlueprintReadOnly, Category = "Delivery")
    bool bGameCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
    float MatchLengthSeconds = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
    TSubclassOf<UUserWidget> EndMatchMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
    int32 ScorePerDelivery = 10;

    UFUNCTION(BlueprintImplementableEvent, Category = "Delivery")
    void BP_OnDeliveryUpdated(int32 Delivered, int32 Required);

    UFUNCTION(BlueprintImplementableEvent, Category = "Delivery")
    void BP_OnGameCompleted();

protected:
    FTimerHandle MatchTimerHandle;

    UFUNCTION()
    void OnMatchTimerTick();
};
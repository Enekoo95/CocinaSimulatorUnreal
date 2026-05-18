// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CocinaSimulatorGameState.generated.h"

UCLASS()
class COCINASIMULATOR_API ACocinaSimulatorGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ACocinaSimulatorGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetTimeRemaining(float NewTime);

    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetSharedScore(int32 NewScore);

    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetDeliveriesRemaining(int32 NewRemaining);

    UFUNCTION(BlueprintPure, Category = "Game")
    float GetTimeRemaining() const { return TimeRemaining; }

    UFUNCTION(BlueprintPure, Category = "Game")
    int32 GetSharedScore() const { return SharedScore; }

    UFUNCTION(BlueprintPure, Category = "Game")
    int32 GetDeliveriesRemaining() const { return DeliveriesRemaining; }

    UFUNCTION(BlueprintPure, Category = "Game")
    bool IsGameOver() const { return bGameOver; }

    UPROPERTY(ReplicatedUsing = OnRep_TimeRemaining, BlueprintReadOnly, Category = "Game")
    float TimeRemaining = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_Score, BlueprintReadOnly, Category = "Game")
    int32 SharedScore = 0;

    UPROPERTY(ReplicatedUsing = OnRep_DeliveriesRemaining, BlueprintReadOnly, Category = "Game")
    int32 DeliveriesRemaining = 0;

    UPROPERTY(ReplicatedUsing = OnRep_GameOver, BlueprintReadOnly, Category = "Game")
    bool bGameOver = false;

    UFUNCTION(BlueprintImplementableEvent, Category = "Game")
    void BP_OnTimerUpdated(float NewTime);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game")
    void BP_OnScoreUpdated(int32 NewScore);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game")
    void BP_OnDeliveriesUpdated(int32 RemainingDeliveries);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game")
    void BP_OnMatchEnded();

protected:
    UFUNCTION()
    void OnRep_TimeRemaining();

    UFUNCTION()
    void OnRep_Score();

    UFUNCTION()
    void OnRep_DeliveriesRemaining();

    UFUNCTION()
    void OnRep_GameOver();
};

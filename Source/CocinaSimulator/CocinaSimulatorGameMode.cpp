// Copyright Epic Games, Inc. All Rights Reserved.

#include "CocinaSimulatorGameMode.h"
#include "CocinaSimulatorPlayerController.h"
#include "CocinaSimulatorGameState.h"
#include "CocinaSimulatorPlayerState.h"
#include "PickUp.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACocinaSimulatorGameMode::ACocinaSimulatorGameMode()
{
    GameStateClass = ACocinaSimulatorGameState::StaticClass();
    PlayerStateClass = ACocinaSimulatorPlayerState::StaticClass();
    DeliveriesCompleted = 0;
    RequiredDeliveries = 3;
    bGameCompleted = false;
}

void ACocinaSimulatorGameMode::BeginPlay()
{
    Super::BeginPlay();

    DeliveriesCompleted = 0;
    bGameCompleted = false;

    if (ACocinaSimulatorGameState* GS = GetGameState<ACocinaSimulatorGameState>())
    {
        GS->SetDeliveriesRemaining(RequiredDeliveries);
        GS->SetSharedScore(0);
        GS->SetTimeRemaining(MatchLengthSeconds);
    }

    BP_OnDeliveryUpdated(DeliveriesCompleted, RequiredDeliveries);
    GetWorldTimerManager().SetTimer(MatchTimerHandle, this, &ACocinaSimulatorGameMode::OnMatchTimerTick, 1.f, true);
}

void ACocinaSimulatorGameMode::AddDelivery(APickUp* Item)
{
    if (bGameCompleted)
    {
        return;
    }

    DeliveriesCompleted++;

    if (ACocinaSimulatorGameState* GS = GetGameState<ACocinaSimulatorGameState>())
    {
        GS->SetDeliveriesRemaining(FMath::Max(0, RequiredDeliveries - DeliveriesCompleted));
    }

    BP_OnDeliveryUpdated(DeliveriesCompleted, RequiredDeliveries);

    if (DeliveriesCompleted >= RequiredDeliveries)
    {
        CompleteMatch(EEndMatchReason::RecipeCompleted);
    }
}

int32 ACocinaSimulatorGameMode::GetRemainingDeliveries() const
{
    return FMath::Max(0, RequiredDeliveries - DeliveriesCompleted);
}

void ACocinaSimulatorGameMode::OnMatchTimerTick()
{
    if (bGameCompleted)
    {
        GetWorldTimerManager().ClearTimer(MatchTimerHandle);
        return;
    }

    if (ACocinaSimulatorGameState* GS = GetGameState<ACocinaSimulatorGameState>())
    {
        float Remaining = FMath::Max(0.f, GS->GetTimeRemaining() - 1.f);
        GS->SetTimeRemaining(Remaining);

        if (Remaining <= 0.f)
        {
            CompleteMatch(EEndMatchReason::TimeExpired);
        }
    }
}

void ACocinaSimulatorGameMode::CompleteMatch(EEndMatchReason Reason)
{
    if (bGameCompleted)
    {
        return;
    }

    bGameCompleted = true;
    GetWorldTimerManager().ClearTimer(MatchTimerHandle);

    if (ACocinaSimulatorGameState* GS = GetGameState<ACocinaSimulatorGameState>())
    {
        GS->bGameOver = true;
    }

    BP_OnGameCompleted();
    ShowEndMatchForPlayers(Reason);
}

void ACocinaSimulatorGameMode::ShowEndMatchForPlayers(EEndMatchReason Reason)
{
    int32 FinalScore = 0;
    if (ACocinaSimulatorGameState* GS = GetGameState<ACocinaSimulatorGameState>())
    {
        FinalScore = GS->GetSharedScore();
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ACocinaSimulatorPlayerController* PC = Cast<ACocinaSimulatorPlayerController>(It->Get()))
        {
            PC->Client_ShowEndMatchMenu(Reason, FinalScore);
        }
    }
}
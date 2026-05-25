// Copyright Epic Games, Inc. All Rights Reserved.
#include "CocinaSimulatorGameState.h"
#include "Net/UnrealNetwork.h"

ACocinaSimulatorGameState::ACocinaSimulatorGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACocinaSimulatorGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACocinaSimulatorGameState, TimeRemaining);
	DOREPLIFETIME(ACocinaSimulatorGameState, SharedScore);
	DOREPLIFETIME(ACocinaSimulatorGameState, DeliveriesRemaining);
	DOREPLIFETIME(ACocinaSimulatorGameState, bGameOver);
	DOREPLIFETIME(ACocinaSimulatorGameState, DeliveredIngredients);
}

// =============================================================================
// Setters
// =============================================================================

void ACocinaSimulatorGameState::SetTimeRemaining(float NewTime)
{
	if (!HasAuthority()) return;
	TimeRemaining = NewTime;
	OnRep_TimeRemaining();
}

void ACocinaSimulatorGameState::SetSharedScore(int32 NewScore)
{
	if (!HasAuthority()) return;
	SharedScore = NewScore;
	OnRep_Score();
}

void ACocinaSimulatorGameState::SetDeliveriesRemaining(int32 NewRemaining)
{
	if (!HasAuthority()) return;
	DeliveriesRemaining = NewRemaining;
	OnRep_DeliveriesRemaining();
}

void ACocinaSimulatorGameState::AddDeliveredIngredient(EIngredientType Type)
{
	if (!HasAuthority()) return;
	if (Type == EIngredientType::None) return;

	// Evitar duplicados
	if (DeliveredIngredients.Contains(Type)) return;

	DeliveredIngredients.Add(Type);

	// Llamada manual en servidor (RepNotify no se dispara en autoridad)
	OnRep_DeliveredIngredients();
}

void ACocinaSimulatorGameState::ClearDeliveredIngredients()
{
	if (!HasAuthority()) return;

	DeliveredIngredients.Empty();
	OnRep_DeliveredIngredients();
}

// =============================================================================
// Getters
// =============================================================================

bool ACocinaSimulatorGameState::HasDeliveredIngredient(EIngredientType Type) const
{
	return DeliveredIngredients.Contains(Type);
}

// =============================================================================
// RepNotify
// =============================================================================

void ACocinaSimulatorGameState::OnRep_TimeRemaining()
{
	BP_OnTimerUpdated(TimeRemaining);
}

void ACocinaSimulatorGameState::OnRep_Score()
{
	BP_OnScoreUpdated(SharedScore);
}

void ACocinaSimulatorGameState::OnRep_DeliveriesRemaining()
{
	BP_OnDeliveriesUpdated(DeliveriesRemaining);
}

void ACocinaSimulatorGameState::OnRep_GameOver()
{
	if (bGameOver)
	{
		BP_OnMatchEnded();
	}
}

void ACocinaSimulatorGameState::OnRep_DeliveredIngredients()
{
	BP_OnDeliveredIngredientsUpdated(DeliveredIngredients);
}
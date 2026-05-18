// Copyright Epic Games, Inc. All Rights Reserved.

#include "CocinaSimulatorPlayerState.h"
#include "Net/UnrealNetwork.h"

ACocinaSimulatorPlayerState::ACocinaSimulatorPlayerState()
{
    PlayerNickname = TEXT("");
}

void ACocinaSimulatorPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACocinaSimulatorPlayerState, PlayerNickname);
}

void ACocinaSimulatorPlayerState::SetPlayerNickname(const FString& NewName)
{
    if (!HasAuthority())
    {
        return;
    }

    PlayerNickname = NewName;
    SetPlayerName(NewName);
    OnRep_PlayerNickname();
}

FString ACocinaSimulatorPlayerState::GetPlayerNickname() const
{
    return PlayerNickname.IsEmpty() ? GetPlayerName() : PlayerNickname;
}

void ACocinaSimulatorPlayerState::OnRep_PlayerNickname()
{
    if (PlayerNickname.IsEmpty())
    {
        PlayerNickname = GetPlayerName();
    }
}

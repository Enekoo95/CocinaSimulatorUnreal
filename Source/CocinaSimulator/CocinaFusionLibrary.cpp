// Copyright Epic Games, Inc. All Rights Reserved.

#include "CocinaFusionLibrary.h"
#include "Engine/GameInstance.h"

UFusionOnlineSubsystem* UCocinaFusionLibrary::GetFusionSubsystem(UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (UGameInstance* GameInstance = WorldContextObject->GetWorld()->GetGameInstance())
    {
        return GameInstance->GetSubsystem<UFusionOnlineSubsystem>();
    }

    return nullptr;
}

UFusionConnectToPhotonAsync* UCocinaFusionLibrary::ConnectToPhoton(UObject* WorldContextObject, const FFusionConnectOptions Options)
{
    if (UFusionOnlineSubsystem* Fusion = GetFusionSubsystem(WorldContextObject))
    {
        return Fusion->ConnectToPhoton(Options, WorldContextObject);
    }
    return nullptr;
}

UFusionJoinOrCreateRoomAsync* UCocinaFusionLibrary::JoinOrCreateRoom(UObject* WorldContextObject, const FFusionRoomOptions Options)
{
    if (UFusionOnlineSubsystem* Fusion = GetFusionSubsystem(WorldContextObject))
    {
        return Fusion->JoinOrCreateRoom(Options, WorldContextObject);
    }
    return nullptr;
}

UFusionJoinRoomAsync* UCocinaFusionLibrary::JoinRoom(UObject* WorldContextObject, const FString RoomName)
{
    if (UFusionOnlineSubsystem* Fusion = GetFusionSubsystem(WorldContextObject))
    {
        return Fusion->JoinRoom(RoomName, WorldContextObject);
    }
    return nullptr;
}

UFusionLeaveRoomAsync* UCocinaFusionLibrary::LeaveRoom(UObject* WorldContextObject)
{
    if (UFusionOnlineSubsystem* Fusion = GetFusionSubsystem(WorldContextObject))
    {
        return Fusion->LeaveRoom(WorldContextObject);
    }
    return nullptr;
}

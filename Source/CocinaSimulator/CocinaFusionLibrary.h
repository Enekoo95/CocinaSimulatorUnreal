// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FusionOnlineSubsystem.h"
#include "CocinaFusionLibrary.generated.h"

UCLASS()
class COCINASIMULATOR_API UCocinaFusionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Photon Fusion", meta = (WorldContext = "WorldContextObject"))
    static UFusionOnlineSubsystem* GetFusionSubsystem(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Photon Fusion", meta = (WorldContext = "WorldContextObject"))
    static UFusionConnectToPhotonAsync* ConnectToPhoton(UObject* WorldContextObject, const FFusionConnectOptions Options);

    UFUNCTION(BlueprintCallable, Category = "Photon Fusion", meta = (WorldContext = "WorldContextObject"))
    static UFusionJoinOrCreateRoomAsync* JoinOrCreateRoom(UObject* WorldContextObject, const FFusionRoomOptions Options);

    UFUNCTION(BlueprintCallable, Category = "Photon Fusion", meta = (WorldContext = "WorldContextObject"))
    static UFusionJoinRoomAsync* JoinRoom(UObject* WorldContextObject, const FString RoomName);

    UFUNCTION(BlueprintCallable, Category = "Photon Fusion", meta = (WorldContext = "WorldContextObject"))
    static UFusionLeaveRoomAsync* LeaveRoom(UObject* WorldContextObject);
};

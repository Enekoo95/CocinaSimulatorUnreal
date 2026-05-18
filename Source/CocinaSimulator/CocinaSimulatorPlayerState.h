// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CocinaSimulatorPlayerState.generated.h"

UCLASS()
class COCINASIMULATOR_API ACocinaSimulatorPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ACocinaSimulatorPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Player")
    void SetPlayerNickname(const FString& NewName);

    UFUNCTION(BlueprintPure, Category = "Player")
    FString GetPlayerNickname() const;

    UPROPERTY(ReplicatedUsing = OnRep_PlayerNickname, BlueprintReadOnly, Category = "Player")
    FString PlayerNickname;

protected:
    UFUNCTION()
    void OnRep_PlayerNickname();
};

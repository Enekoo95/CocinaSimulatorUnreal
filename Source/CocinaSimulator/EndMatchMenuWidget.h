// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CocinaSimulator.h"
#include "EndMatchMenuWidget.generated.h"

UCLASS()
class COCINASIMULATOR_API UEndMatchMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "EndMatch")
    void SetEndMatchSummary(EEndMatchReason Reason, int32 FinalScore);
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CocinaSimulator.generated.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogCocinaSimulator, Log, All);

UENUM(BlueprintType)
enum class EEndMatchReason : uint8
{
	TimeExpired UMETA(DisplayName = "Tiempo terminado"),
	RecipeCompleted UMETA(DisplayName = "Receta completada")
};
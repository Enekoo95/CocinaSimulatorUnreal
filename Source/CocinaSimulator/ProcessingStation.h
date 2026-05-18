// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickUp.h"
#include "ProcessingStation.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class COCINASIMULATOR_API AProcessingStation : public AActor
{
    GENERATED_BODY()

public:
    AProcessingStation();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category="Station")
    bool ReceiveItem(APickUp* Item);

    UFUNCTION(BlueprintCallable, Category="Station")
    bool CanAcceptItem(APickUp* Item) const;

    UFUNCTION(BlueprintPure, Category="Station")
    EIngredientType GetStationIngredient() const { return StationIngredient; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station")
    EIngredientType StationIngredient = EIngredientType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station")
    float ProcessTimeSeconds = 8.f;

    UPROPERTY(ReplicatedUsing=OnRep_CurrentItem, BlueprintReadOnly, Category="Station")
    APickUp* CurrentItem = nullptr;

    UPROPERTY(ReplicatedUsing=OnRep_IsProcessing, BlueprintReadOnly, Category="Station")
    bool bIsProcessing = false;

    UFUNCTION(BlueprintImplementableEvent, Category="Station")
    void BP_OnProcessingStarted(APickUp* Item);

    UFUNCTION(BlueprintImplementableEvent, Category="Station")
    void BP_OnProcessingCompleted(APickUp* Item);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    UStaticMeshComponent* StationMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    UBoxComponent* ZoneBox;

    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial = nullptr;

    FTimerHandle ProcessTimerHandle;

    void UpdateStationColor();

    UFUNCTION()
    void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnRep_CurrentItem();

    UFUNCTION()
    void OnRep_IsProcessing();

    UFUNCTION()
    void FinishProcessing();
};
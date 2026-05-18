// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProcessingStation.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "CocinaSimulatorGameState.h"
AProcessingStation::AProcessingStation()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
    RootComponent = StationMesh;
    StationMesh->SetCollisionProfileName(TEXT("BlockAll"));

    ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
    ZoneBox->SetupAttachment(RootComponent);
    ZoneBox->SetBoxExtent(FVector(120.f, 120.f, 80.f));
    ZoneBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    ZoneBox->SetGenerateOverlapEvents(true);
    ZoneBox->OnComponentBeginOverlap.AddDynamic(this, &AProcessingStation::OnZoneBeginOverlap);
}

void AProcessingStation::BeginPlay()
{
    Super::BeginPlay();

    if (StationMesh && StationMesh->GetMaterial(0))
    {
        DynamicMaterial = StationMesh->CreateAndSetMaterialInstanceDynamic(0);
        UpdateStationColor();
    }
}

void AProcessingStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AProcessingStation, CurrentItem);
    DOREPLIFETIME(AProcessingStation, bIsProcessing);
}

bool AProcessingStation::CanAcceptItem(APickUp* Item) const
{
    return Item != nullptr && !bIsProcessing && Item->IngredientType == StationIngredient && Item->ItemState == EItemState::Raw;
}

bool AProcessingStation::ReceiveItem(APickUp* Item)
{
    if (!HasAuthority() || !CanAcceptItem(Item))
    {
        return false;
    }

    CurrentItem = Item;
    bIsProcessing = true;

    if (CurrentItem)
    {
        CurrentItem->PlaceInStation(GetActorLocation());
    }

    UpdateStationColor();
    GetWorldTimerManager().SetTimer(ProcessTimerHandle, this, &AProcessingStation::FinishProcessing, ProcessTimeSeconds, false);
    BP_OnProcessingStarted(CurrentItem);
    return true;
}
void AProcessingStation::FinishProcessing()
{
    if (!CurrentItem)
    {
        bIsProcessing = false;
        UpdateStationColor();
        return;
    }

    CurrentItem->MarkReady();
    CurrentItem->SetActorLocation(GetActorLocation() + FVector(0.f, 0.f, 20.f));

    // Sumar puntos al procesar el ingrediente
    if (ACocinaSimulatorGameState* GS = Cast<ACocinaSimulatorGameState>(GetWorld()->GetGameState()))
    {
        int32 Points = 0;
        switch (CurrentItem->IngredientType)
        {
        case EIngredientType::Meat:
            Points = 10;
            break;
        case EIngredientType::Lettuce:
            Points = 20;
            break;
        case EIngredientType::Potato:
            Points = 30;
            break;
        default:
            Points = 10;
            break;
        }
        GS->SetSharedScore(GS->GetSharedScore() + Points);
    }

    BP_OnProcessingCompleted(CurrentItem);
    CurrentItem = nullptr;
    bIsProcessing = false;
    UpdateStationColor();
}
void AProcessingStation::UpdateStationColor()
{
    if (!DynamicMaterial)
    {
        return;
    }

    FLinearColor NewColor;

    if (bIsProcessing)
    {
        // Amarillo: procesando
        NewColor = FLinearColor(1.f, 0.85f, 0.f, 1.f);
    }
    else if (CurrentItem != nullptr && CurrentItem->ItemState == EItemState::Ready)
    {
        // Rojo: listo para recoger
        NewColor = FLinearColor(1.f, 0.15f, 0.15f, 1.f);
    }
    else
    {
        // Verde: libre
        NewColor = FLinearColor(0.1f, 0.85f, 0.2f, 1.f);
    }

    DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), NewColor);
}

void AProcessingStation::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (APickUp* Pickup = Cast<APickUp>(OtherActor))
    {
        ReceiveItem(Pickup);
    }
}

void AProcessingStation::OnRep_CurrentItem()
{
    UpdateStationColor();
}

void AProcessingStation::OnRep_IsProcessing()
{
    UpdateStationColor();
}
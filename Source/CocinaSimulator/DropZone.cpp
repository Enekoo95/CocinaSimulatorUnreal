// Fill out your copyright notice in the Description page of Project Settings.

#include "DropZone.h"
#include "Components/BoxComponent.h"
#include "PickUp.h"
#include "CocinaSimulatorGameMode.h"
#include "CocinaSimulatorGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ADropZone::ADropZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
	ZoneBox->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	ZoneBox->SetCollisionProfileName(TEXT("OverlapAll"));
	ZoneBox->SetGenerateOverlapEvents(true);
	ZoneBox->OnComponentBeginOverlap.AddDynamic(this, &ADropZone::OnZoneBeginOverlap);
	RootComponent = ZoneBox;
}

void ADropZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADropZone, DeliveredItems);
}

void ADropZone::ReceiveItem(APickUp* Item)
{
	if (!HasAuthority()) return;

	if (!Item || Item->bWasDelivered || Item->ItemState != EItemState::Ready)
	{
		return;
	}

	if (HasIngredient(Item->IngredientType))
	{
		return;
	}

	if (Item->bIsHeld)
	{
		Item->Drop(GetActorLocation(), true);
	}

	Item->bWasDelivered = true;
	DeliveredItems.Add(Item);

	Multicast_OnItemReceived(Item);
	CheckRecipeComplete();
}

bool ADropZone::HasIngredient(EIngredientType Type) const
{
	for (APickUp* Item : DeliveredItems)
	{
		if (Item && Item->IngredientType == Type)
		{
			return true;
		}
	}
	return false;
}

void ADropZone::OnRep_DeliveredItems()
{
	BP_OnDeliveredItemsUpdated(DeliveredItems);
}

void ADropZone::CheckRecipeComplete()
{
	bool bHasMeat = HasIngredient(EIngredientType::Meat);
	bool bHasLettuce = HasIngredient(EIngredientType::Lettuce);
	bool bHasPotato = HasIngredient(EIngredientType::Potato);

	if (bHasMeat && bHasLettuce && bHasPotato)
	{
		if (ACocinaSimulatorGameMode* GM = Cast<ACocinaSimulatorGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			if (ACocinaSimulatorGameState* GS = GM->GetGameState<ACocinaSimulatorGameState>())
			{
				GS->SetSharedScore(GS->GetSharedScore() + 100);
			}

			GM->AddDelivery(nullptr);
		}

		for (APickUp* Item : DeliveredItems)
		{
			if (Item)
			{
				Item->SetActorHiddenInGame(true);
				Item->SetActorEnableCollision(false);
				Item->Destroy();
			}
		}
		DeliveredItems.Empty();

		Multicast_OnRecipeCompleted();
	}
}

void ADropZone::Multicast_OnItemReceived_Implementation(APickUp* Item)
{
	BP_OnItemReceived(Item);
}

void ADropZone::Multicast_OnRecipeCompleted_Implementation()
{
	BP_OnRecipeCompleted();
}

void ADropZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APickUp* Item = Cast<APickUp>(OtherActor))
	{
		ReceiveItem(Item);
	}
}
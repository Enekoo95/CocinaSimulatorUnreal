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
	// Solo el servidor ejecuta esta lógica
	if (!HasAuthority()) return;
	if (!Item)           return;

	// Validaciones: ya entregado, no está listo, o ya tenemos ese ingrediente
	if (Item->bWasDelivered)                          return;
	if (Item->ItemState != EItemState::Ready)          return;
	if (HasIngredient(Item->IngredientType))           return;

	// Si el jugador todavía lo lleva en la mano, soltarlo en la zona
	if (Item->bIsHeld)
	{
		// DropItem con bInDropZone=true: lo coloca aquí y marca como Delivered
		Item->DropItem(GetActorLocation(), true);
	}
	else
	{
		// Ya estaba suelto en el mundo pero dentro de la zona (overlap)
		Item->SetItemState(EItemState::Delivered);
		Item->bWasDelivered = true;
		Item->SetActorLocation(GetActorLocation());
		Item->SetActorEnableCollision(false);
	}

	DeliveredItems.Add(Item);

	// Notificar a todos los clientes visualmente
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

void ADropZone::CheckRecipeComplete()
{
	// Solo el servidor llama esto (viene de ReceiveItem que ya valida autoridad)
	const bool bHasMeat = HasIngredient(EIngredientType::Meat);
	const bool bHasLettuce = HasIngredient(EIngredientType::Lettuce);
	const bool bHasPotato = HasIngredient(EIngredientType::Potato);

	if (!bHasMeat || !bHasLettuce || !bHasPotato) return;

	// Receta completa — sumar puntos y limpiar
	if (ACocinaSimulatorGameMode* GM = Cast<ACocinaSimulatorGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (ACocinaSimulatorGameState* GS = GM->GetGameState<ACocinaSimulatorGameState>())
		{
			GS->SetSharedScore(GS->GetSharedScore() + 100);
		}
		GM->AddDelivery(nullptr);
	}

	// Destruir ingredientes en el servidor (se replica a clientes)
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



void ADropZone::Multicast_OnItemReceived_Implementation(APickUp* Item)
{
	BP_OnItemReceived(Item);
}

void ADropZone::Multicast_OnRecipeCompleted_Implementation()
{
	BP_OnRecipeCompleted();
}



void ADropZone::OnRep_DeliveredItems()
{
	BP_OnDeliveredItemsUpdated(DeliveredItems);
}


void ADropZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// OnZoneBeginOverlap se dispara en todos los clientes pero ReceiveItem
	// tiene la guarda HasAuthority(), así que es seguro llamarlo aquí.
	if (APickUp* Item = Cast<APickUp>(OtherActor))
	{
		ReceiveItem(Item);
	}
}
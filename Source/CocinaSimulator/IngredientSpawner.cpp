// Fill out your copyright notice in the Description page of Project Settings.
#include "IngredientSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

AIngredientSpawner::AIngredientSpawner()
{
	PrimaryActorTick.bCanEverTick = false; // FIX: no necesitamos Tick, usamos callbacks
	bReplicates = true;

	SpawnerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpawnerMesh"));
	RootComponent = SpawnerMesh;
	SpawnerMesh->SetCollisionProfileName(TEXT("BlockAll"));

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(RootComponent);
	SpawnPoint->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
}

void AIngredientSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnIngredient();
	}
}

void AIngredientSpawner::SpawnIngredient()
{
	if (!IngredientClass || CurrentItem != nullptr) return;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APickUp* NewItem = GetWorld()->SpawnActor<APickUp>(
		IngredientClass,
		SpawnPoint->GetComponentLocation(),
		GetActorRotation(),
		Params
	);

	if (NewItem)
	{
		CurrentItem = NewItem;
		// Decirle al item que conoce a su spawner para callbacks bidireccionales
		CurrentItem->OwnerSpawner = this;
	}
}

// El item nos avisa cuando fue cogido -> iniciar timer de respawn
void AIngredientSpawner::NotifyItemTaken(APickUp* Item)
{
	if (!HasAuthority()) return;
	if (Item != CurrentItem) return;

	CurrentItem = nullptr;

	GetWorldTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&AIngredientSpawner::SpawnIngredient,
		RespawnDelay,
		false
	);
}


void AIngredientSpawner::NotifyItemReturned(APickUp* Item)
{
	if (!HasAuthority()) return;

	// Si ya habia otro item (respawn ocurrio antes), ignorar
	if (CurrentItem != nullptr && CurrentItem != Item) return;

	// Cancelar timer de respawn si aun no habia spawneado
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	CurrentItem = Item;
}
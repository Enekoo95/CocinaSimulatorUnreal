#include "IngredientSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

AIngredientSpawner::AIngredientSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
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

void AIngredientSpawner::Tick(float DeltaTime)  
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	CheckItemTaken();
}

void AIngredientSpawner::SpawnIngredient()
{
	if (!IngredientClass || CurrentItem != nullptr)
	{
		return;
	}

	FVector SpawnLocation = SpawnPoint->GetComponentLocation();
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CurrentItem = GetWorld()->SpawnActor<APickUp>(IngredientClass, SpawnLocation, SpawnRotation, Params);
}

void AIngredientSpawner::CheckItemTaken()
{
	if (CurrentItem == nullptr)
	{
		return;
	}

	if (CurrentItem->bIsHeld || CurrentItem->IsPendingKillPending())
	{
		CurrentItem = nullptr;
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AIngredientSpawner::SpawnIngredient, RespawnDelay, false);
	}
}
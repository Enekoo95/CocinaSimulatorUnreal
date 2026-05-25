// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickUp.h"
#include "IngredientSpawner.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class COCINASIMULATOR_API AIngredientSpawner : public AActor
{
	GENERATED_BODY()

public:
	AIngredientSpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<APickUp> IngredientClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float RespawnDelay = 2.f;

	// Llamado por APickUp cuando es recogido — notifica al spawner que libere el slot
	void NotifyItemTaken(APickUp* Item);

	// Llamado por APickUp cuando es soltado fuera de DropZone — re-registra el item
	void NotifyItemReturned(APickUp* Item);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SpawnerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SpawnPoint;

	// Item actual spawneado en esta plataforma (null = libre)
	UPROPERTY()
	APickUp* CurrentItem = nullptr;

	FTimerHandle RespawnTimerHandle;

	void SpawnIngredient();
};
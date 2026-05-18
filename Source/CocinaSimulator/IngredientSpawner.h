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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	TSubclassOf<APickUp> IngredientClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	float RespawnDelay = 2.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* SpawnerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* SpawnPoint;

	UPROPERTY()
	APickUp* CurrentItem = nullptr;

	FTimerHandle RespawnTimerHandle;

	void SpawnIngredient();
	void CheckItemTaken();
};
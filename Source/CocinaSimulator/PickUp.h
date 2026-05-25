// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickUp.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AIngredientSpawner;

UENUM(BlueprintType)
enum class EIngredientType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Meat        UMETA(DisplayName = "Meat"),
	Lettuce     UMETA(DisplayName = "Lettuce"),
	Potato      UMETA(DisplayName = "Potato"),
	Plate       UMETA(DisplayName = "Plate")
};

UENUM(BlueprintType)
enum class EItemState : uint8
{
	Raw = 0 UMETA(DisplayName = "Raw"),
	Processing      UMETA(DisplayName = "Processing"),
	Ready           UMETA(DisplayName = "Ready"),
	Delivered       UMETA(DisplayName = "Delivered")
};

UCLASS()
class COCINASIMULATOR_API APickUp : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionSphere;

	UPROPERTY()
	TWeakObjectPtr<USceneComponent> HoldPointRef;

public:
	APickUp();

	// ---- Estado replicado ------------------------------------------------

	UPROPERTY(ReplicatedUsing = OnRep_IsHeld, BlueprintReadOnly, Category = "Pickup")
	bool bIsHeld = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pickup")
	bool bWasDelivered = false;

	UPROPERTY(ReplicatedUsing = OnRep_ItemState, BlueprintReadOnly, Category = "Pickup")
	EItemState ItemState = EItemState::Raw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Pickup")
	EIngredientType IngredientType = EIngredientType::None;

	UPROPERTY(ReplicatedUsing = OnRep_IsOnSpawner, BlueprintReadWrite, Category = "Pickup")
	bool bIsOnSpawner = true;

	// Referencia al spawner que creó este item (solo servidor, no necesita replicarse)
	UPROPERTY()
	AIngredientSpawner* OwnerSpawner = nullptr;

	// ---- Funciones públicas -----------------------------------------------

	void PickUpItem(USceneComponent* HoldPoint);
	void DropItem(FVector DropLocation, bool bInDropZone = false);
	void PlaceInStation(const FVector& Location);
	void MarkReady();
	void SetItemState(EItemState NewState);

	// ---- Blueprint Events ------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void BP_OnPickedUp();

	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void BP_OnDropped(bool bInDropZone);

	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void BP_OnStateChanged(EItemState NewState);

	FORCEINLINE UStaticMeshComponent* GetMesh()              const { return Mesh; }
	FORCEINLINE USphereComponent* GetInteractionSphere() const { return InteractionSphere; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION() void OnRep_ItemState();
	UFUNCTION() void OnRep_IsHeld();
	UFUNCTION() void OnRep_IsOnSpawner();

	UFUNCTION(Server, Reliable) void Server_PickUp(USceneComponent* HoldPoint);
	UFUNCTION(Server, Reliable) void Server_Drop(FVector DropLocation, bool bInDropZone);
	UFUNCTION(Server, Reliable) void Server_PlaceInStation(FVector Location);
	UFUNCTION(Server, Reliable) void Server_MarkReady();

	UFUNCTION(NetMulticast, Reliable) void Multicast_OnPickedUp();
	UFUNCTION(NetMulticast, Reliable) void Multicast_OnDropped(bool bInDropZone);

	void UpdateItemColor();

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial = nullptr;

	float   BobTime = 0.f;
	FVector SpawnerBaseLocation = FVector::ZeroVector;
};
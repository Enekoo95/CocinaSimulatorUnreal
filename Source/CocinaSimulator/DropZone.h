// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickUp.h"
#include "DropZone.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
struct FHitResult;
class APickUp;

UCLASS()
class COCINASIMULATOR_API ADropZone : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* ZoneBox;

public:
	ADropZone();

	UFUNCTION(BlueprintCallable, Category = "DropZone")
	void ReceiveItem(APickUp* Item);

	UFUNCTION(BlueprintPure, Category = "DropZone")
	bool HasIngredient(EIngredientType Type) const;

	UFUNCTION(BlueprintPure, Category = "DropZone")
	int32 GetDeliveredCount() const { return DeliveredItems.Num(); }

	UFUNCTION(BlueprintImplementableEvent, Category = "DropZone")
	void BP_OnItemReceived(APickUp* Item);

	UFUNCTION(BlueprintImplementableEvent, Category = "DropZone")
	void BP_OnRecipeCompleted();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnItemReceived(APickUp* Item);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnRecipeCompleted();

	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	FORCEINLINE UBoxComponent* GetZoneBox() const { return ZoneBox; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintImplementableEvent, Category = "DropZone")
	void BP_OnDeliveredItemsUpdated(const TArray<APickUp*>& Items);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_DeliveredItems, BlueprintReadOnly, Category = "DropZone")
	TArray<APickUp*> DeliveredItems;

	UFUNCTION()
	void OnRep_DeliveredItems();

	void CheckRecipeComplete();
};
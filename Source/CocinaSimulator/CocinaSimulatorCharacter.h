// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CocinaSimulatorCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class APickUp;
class ADropZone;
class AProcessingStation;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class ACocinaSimulatorCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* HoldPoint;

protected:
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* LookAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* MouseLookAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* InteractAction;

	UPROPERTY(ReplicatedUsing = OnRep_HeldItem, BlueprintReadOnly, Category = "Pickup")
	APickUp* HeldItem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float PickupRange = 150.f;

public:
	ACocinaSimulatorCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TryPickup(APickUp* Item);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TryDrop();

	// Mantenemos este para compatibilidad con el flujo existente
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAttemptInteract();

	UFUNCTION()
	void OnRep_HeldItem();

	void Look(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);

	// Helpers internos solo servidor
	void Server_DoPickup(APickUp* Item);
	void Server_DoDrop();

	// Helper de sweep compartido por cliente y servidor
	APickUp* FindNearestPickup() const;

public:
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoMove(float Right, float Forward);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoLook(float Yaw, float Pitch);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpStart();
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpEnd();
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoInteract();

	FORCEINLINE USpringArmComponent* GetCameraBoom()  const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE USceneComponent* GetHoldPoint()    const { return HoldPoint; }
};
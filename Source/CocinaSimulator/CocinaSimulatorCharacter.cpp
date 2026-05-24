// Copyright Epic Games, Inc. All Rights Reserved.

#include "CocinaSimulatorCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "CocinaSimulator.h"
#include "PickUp.h"
#include "DropZone.h"
#include "ProcessingStation.h"
#include "Net/UnrealNetwork.h"



ACocinaSimulatorCharacter::ACocinaSimulatorCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HoldPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HoldPoint"));
	HoldPoint->SetupAttachment(GetRootComponent());
	HoldPoint->SetRelativeLocation(FVector(80.f, 0.f, 50.f));

	bReplicates = true;
	SetReplicateMovement(true);
}


void ACocinaSimulatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Look);

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ACocinaSimulatorCharacter::DoInteract);
		}
	}
	else
	{
		UE_LOG(LogCocinaSimulator, Error, TEXT("'%s' No se encontró Enhanced Input Component!"), *GetNameSafe(this));
	}
}

void ACocinaSimulatorCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACocinaSimulatorCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACocinaSimulatorCharacter::DoMove(float Right, float Forward)
{
	if (!GetController()) return;

	const FRotator Rotation = GetController()->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector  ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector  RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDir, Forward);
	AddMovementInput(RightDir, Right);
}

void ACocinaSimulatorCharacter::DoLook(float Yaw, float Pitch)
{
	if (!GetController()) return;
	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void ACocinaSimulatorCharacter::DoJumpStart() { Jump(); }
void ACocinaSimulatorCharacter::DoJumpEnd() { StopJumping(); }



void ACocinaSimulatorCharacter::DoInteract()
{
	// El cliente NO ejecuta lógica de juego directamente.
	// Solo envía la petición al servidor.
	if (!HasAuthority())
	{
		ServerAttemptInteract();
		return;
	}

	// A partir de aquí solo el servidor ejecuta esto.

	if (HeldItem)
	{

		// Restaurar rotación libre
		GetCharacterMovement()->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;

		TArray<AActor*> OverlappingActors;

		// 1. ¿Estamos sobre una ProcessingStation?
		GetOverlappingActors(OverlappingActors, AProcessingStation::StaticClass());
		bool bDeliveredToStation = false;

		if (OverlappingActors.Num() > 0)
		{
			AProcessingStation* Station = Cast<AProcessingStation>(OverlappingActors[0]);
			if (Station && Station->ReceiveItem(HeldItem))
			{
				// ReceiveItem llama internamente a PlaceInStation, que ya es RPC-safe
				HeldItem = nullptr;
				bDeliveredToStation = true;
			}
		}

		if (!bDeliveredToStation)
		{
			// 2. ¿Estamos sobre una DropZone?
			GetOverlappingActors(OverlappingActors, ADropZone::StaticClass());
			if (OverlappingActors.Num() > 0)
			{
				ADropZone* Zone = Cast<ADropZone>(OverlappingActors[0]);
				if (Zone)
				{
					Zone->ReceiveItem(HeldItem);
					// ReceiveItem llama DropItem internamente (ver DropZone.cpp)
					HeldItem = nullptr;
				}
			}
			else
			{
				// 3. Soltar al suelo
				HeldItem->DropItem(HoldPoint->GetComponentLocation());
				HeldItem = nullptr;
			}
		}
	}
	else
	{

		FVector Start = GetActorLocation();
		FVector End = Start + GetActorForwardVector() * PickupRange;

		TArray<FHitResult> Hits;
		FCollisionShape    Sphere = FCollisionShape::MakeSphere(60.f);

		FCollisionObjectQueryParams ObjQuery;
		ObjQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjQuery.AddObjectTypesToQuery(ECC_PhysicsBody);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjQuery, Sphere, Params);

		for (const FHitResult& Hit : Hits)
		{
			APickUp* Pickup = Cast<APickUp>(Hit.GetActor());
			if (Pickup && !Pickup->bIsHeld)
			{
				// Llamamos PickUpItem — ya gestiona el Server RPC internamente
				Pickup->PickUpItem(HoldPoint);
				HeldItem = Pickup;

				// Mientras lleva algo: el personaje mira hacia donde apunta la cámara
				GetCharacterMovement()->bOrientRotationToMovement = false;
				bUseControllerRotationYaw = true;
				break;
			}
		}
	}
}

void ACocinaSimulatorCharacter::ServerAttemptInteract_Implementation()
{
	DoInteract();
}

bool ACocinaSimulatorCharacter::ServerAttemptInteract_Validate()
{
	return true;
}



void ACocinaSimulatorCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACocinaSimulatorCharacter, HeldItem);
}

void ACocinaSimulatorCharacter::OnRep_HeldItem()
{
	if (HeldItem)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}
	else
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
	}
}
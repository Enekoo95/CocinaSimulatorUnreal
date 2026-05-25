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

// =============================================================================
// Input binding
// =============================================================================
void ACocinaSimulatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Move);
		EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Look);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Look);

		if (InteractAction)
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ACocinaSimulatorCharacter::DoInteract);
	}
}

void ACocinaSimulatorCharacter::Move(const FInputActionValue& Value)
{
	FVector2D V = Value.Get<FVector2D>();
	DoMove(V.X, V.Y);
}

void ACocinaSimulatorCharacter::Look(const FInputActionValue& Value)
{
	FVector2D V = Value.Get<FVector2D>();
	DoLook(V.X, V.Y);
}

void ACocinaSimulatorCharacter::DoMove(float Right, float Forward)
{
	if (!GetController()) return;
	const FRotator Yaw(0, GetController()->GetControlRotation().Yaw, 0);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Forward);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Right);
}

void ACocinaSimulatorCharacter::DoLook(float Yaw, float Pitch)
{
	if (!GetController()) return;
	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void ACocinaSimulatorCharacter::DoJumpStart() { Jump(); }
void ACocinaSimulatorCharacter::DoJumpEnd() { StopJumping(); }

// =============================================================================
// Helpers de sweep — compartido por cliente y servidor
// =============================================================================
APickUp* ACocinaSimulatorCharacter::FindNearestPickup() const
{
	FCollisionShape Sphere = FCollisionShape::MakeSphere(60.f);
	FCollisionObjectQueryParams ObjQ;
	ObjQ.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjQ.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(
		Hits,
		GetActorLocation(),
		GetActorLocation() + GetActorForwardVector() * PickupRange,
		FQuat::Identity, ObjQ, Sphere, Params
	);

	for (const FHitResult& Hit : Hits)
	{
		APickUp* P = Cast<APickUp>(Hit.GetActor());
		if (P && !P->bIsHeld)
			return P;
	}
	return nullptr;
}

// =============================================================================
// DoInteract
// =============================================================================
void ACocinaSimulatorCharacter::DoInteract()
{
	if (HasAuthority())
	{
		if (HeldItem)
			Server_DoDrop();
		else
		{
			APickUp* Found = FindNearestPickup();
			if (Found)
				Server_DoPickup(Found);
		}
		return;
	}

	// Cliente
	if (HeldItem)
	{
		Server_TryDrop();
	}
	else
	{
		APickUp* Found = FindNearestPickup();
		if (Found)
			Server_TryPickup(Found);
	}
}

// =============================================================================
// Server RPCs
// =============================================================================
void ACocinaSimulatorCharacter::Server_TryPickup_Implementation(APickUp* Item)
{
	if (!Item || Item->bIsHeld) return;
	Server_DoPickup(Item);
}

bool ACocinaSimulatorCharacter::Server_TryPickup_Validate(APickUp* Item)
{
	return true;
}

void ACocinaSimulatorCharacter::Server_TryDrop_Implementation()
{
	if (!HeldItem) return;
	Server_DoDrop();
}

bool ACocinaSimulatorCharacter::Server_TryDrop_Validate()
{
	return true;
}

void ACocinaSimulatorCharacter::ServerAttemptInteract_Implementation()
{
	DoInteract();
}

bool ACocinaSimulatorCharacter::ServerAttemptInteract_Validate()
{
	return true;
}

// =============================================================================
// Helpers internos servidor
// =============================================================================
void ACocinaSimulatorCharacter::Server_DoPickup(APickUp* Item)
{
	if (!Item || Item->bIsHeld || !HoldPoint) return;

	// FIX PRINCIPAL: pasar 'this' para que HoldingCharacter se replique
	// y OnRep_IsHeld en los clientes pueda hacer el attach visual correcto
	Item->PickUpItem(HoldPoint, this);
	HeldItem = Item;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
}

void ACocinaSimulatorCharacter::Server_DoDrop()
{
	if (!HeldItem) return;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	TArray<AActor*> Overlapping;

	// 1. ¿Sobre una ProcessingStation?
	GetOverlappingActors(Overlapping, AProcessingStation::StaticClass());
	if (Overlapping.Num() > 0)
	{
		if (AProcessingStation* Station = Cast<AProcessingStation>(Overlapping[0]))
		{
			if (Station->ReceiveItem(HeldItem))
			{
				HeldItem = nullptr;
				return;
			}
		}
	}

	// 2. ¿Sobre una DropZone?
	GetOverlappingActors(Overlapping, ADropZone::StaticClass());
	if (Overlapping.Num() > 0)
	{
		if (ADropZone* Zone = Cast<ADropZone>(Overlapping[0]))
		{
			Zone->ReceiveItem(HeldItem);
			HeldItem = nullptr;
			return;
		}
	}

	// 3. Soltar al suelo
	HeldItem->DropItem(HoldPoint->GetComponentLocation());
	HeldItem = nullptr;
}

// =============================================================================
// Replication
// =============================================================================
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
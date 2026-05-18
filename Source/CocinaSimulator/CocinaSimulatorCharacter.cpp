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

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Hold point positioned in front of the character (80 forward, 50 up from capsule center)
	HoldPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HoldPoint"));
	HoldPoint->SetupAttachment(GetRootComponent());
	HoldPoint->SetRelativeLocation(FVector(80.f, 0.f, 50.f));

	bReplicates = true;
	SetReplicateMovement(true);
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ACocinaSimulatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACocinaSimulatorCharacter::Look);

		// Interact / Pickup
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ACocinaSimulatorCharacter::DoInteract);
		}
	}
	else
	{
		UE_LOG(LogCocinaSimulator, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACocinaSimulatorCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACocinaSimulatorCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACocinaSimulatorCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ACocinaSimulatorCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACocinaSimulatorCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ACocinaSimulatorCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ACocinaSimulatorCharacter::DoInteract()
{
	if (!HasAuthority())
	{
		ServerAttemptInteract();
		return;
	}

	if (HeldItem)
	{
		// Restore original rotation mode
		GetCharacterMovement()->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;

		// Check if player is standing inside a processing station first
		TArray<AActor*> OverlappingActors;
		GetOverlappingActors(OverlappingActors, AProcessingStation::StaticClass());

		bool bDeliveredToStation = false;
		if (OverlappingActors.Num() > 0)
		{
			AProcessingStation* Station = Cast<AProcessingStation>(OverlappingActors[0]);
			if (Station && Station->ReceiveItem(HeldItem))
			{
				HeldItem = nullptr;
				bDeliveredToStation = true;
			}
		}

		if (!bDeliveredToStation)
		{
			GetOverlappingActors(OverlappingActors, ADropZone::StaticClass());
			if (OverlappingActors.Num() > 0)
			{
				ADropZone* Zone = Cast<ADropZone>(OverlappingActors[0]);
				Zone->ReceiveItem(HeldItem);
				HeldItem = nullptr;
			}
			else
			{
				HeldItem->Drop(HoldPoint->GetComponentLocation());
				HeldItem = nullptr;
			}
		}
	}
	else
	{
		// Sweep a sphere forward to find a nearby pickup
		FVector Start = GetActorLocation();
		FVector End = Start + GetActorForwardVector() * PickupRange;

		TArray<FHitResult> Hits;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(60.f);
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
				Pickup->PickUp(HoldPoint);
				HeldItem = Pickup;

				// While holding: character faces camera so the item is always visible in front
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
	// Local clients }
}
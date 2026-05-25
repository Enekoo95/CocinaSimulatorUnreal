// Fill out your copyright notice in the Description page of Project Settings.
#include "PickUp.h"
#include "IngredientSpawner.h"
#include "CocinaSimulatorCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

APickUp::APickUp()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(80.f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void APickUp::BeginPlay()
{
	Super::BeginPlay();
	SpawnerBaseLocation = GetActorLocation();

	if (bIsOnSpawner)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	UpdateItemColor();
}

void APickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bIsOnSpawner && !bIsHeld)
	{
		BobTime += DeltaTime;
		float BobOffset = FMath::Sin(BobTime * 2.f) * 10.f;
		SetActorLocation(SpawnerBaseLocation + FVector(0.f, 0.f, BobOffset));
		AddActorLocalRotation(FRotator(0.f, 45.f * DeltaTime, 0.f));
	}
}

void APickUp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APickUp, bIsHeld);
	DOREPLIFETIME(APickUp, bWasDelivered);
	DOREPLIFETIME(APickUp, ItemState);
	DOREPLIFETIME(APickUp, IngredientType);
	DOREPLIFETIME(APickUp, bIsOnSpawner);
	DOREPLIFETIME(APickUp, HoldingCharacter);
}

void APickUp::SetItemState(EItemState NewState)
{
	if (!HasAuthority()) return;
	if (ItemState == NewState) return;

	ItemState = NewState;
	OnRep_ItemState();
}

// =============================================================================
// Helpers de fisica
// =============================================================================

void APickUp::ApplyPickupPhysics()
{
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetReplicateMovement(false);
}

void APickUp::ApplyDropPhysics()
{
	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetReplicateMovement(true);
}

void APickUp::ApplyDeliveredPhysics()
{
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// =============================================================================
// PickUp
// =============================================================================

void APickUp::PickUpItem(USceneComponent* HoldPoint, ACocinaSimulatorCharacter* Character)
{
	if (!HoldPoint || bIsHeld) return;

	if (HasAuthority())
		Server_PickUp_Implementation(HoldPoint, Character);
	else
		Server_PickUp(HoldPoint, Character);
}

void APickUp::Server_PickUp_Implementation(USceneComponent* HoldPoint, ACocinaSimulatorCharacter* Character)
{
	if (!HoldPoint || bIsHeld) return;

	bIsHeld = true;
	bIsOnSpawner = false;
	bWasDelivered = false;
	HoldingCharacter = Character;
	HoldPointRef = HoldPoint;

	ApplyPickupPhysics();

	AttachToComponent(HoldPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	if (OwnerSpawner)
	{
		OwnerSpawner->NotifyItemTaken(this);
	}

	Multicast_OnPickedUp();
}

// =============================================================================
// Drop
// =============================================================================

void APickUp::DropItem(FVector DropLocation, bool bInDropZone)
{
	if (!bIsHeld) return;

	if (HasAuthority())
		Server_Drop_Implementation(DropLocation, bInDropZone);
	else
		Server_Drop(DropLocation, bInDropZone);
}

void APickUp::Server_Drop_Implementation(FVector DropLocation, bool bInDropZone)
{
	if (!bIsHeld) return;

	bIsHeld = false;
	HoldingCharacter = nullptr;
	HoldPointRef = nullptr;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (bInDropZone)
	{
		bWasDelivered = true;
		SetItemState(EItemState::Delivered);
		SetActorLocation(DropLocation);
		ApplyDeliveredPhysics();
	}
	else
	{
		SetActorLocation(DropLocation + FVector(0.f, 0.f, 15.f));
		ApplyDropPhysics();

		if (OwnerSpawner)
		{
			OwnerSpawner->NotifyItemReturned(this);
		}
	}

	Multicast_OnDropped(bInDropZone);
}

// =============================================================================
// PlaceInStation
// =============================================================================

void APickUp::PlaceInStation(const FVector& Location)
{
	if (!bIsHeld) return;

	if (HasAuthority())
		Server_PlaceInStation_Implementation(Location);
	else
		Server_PlaceInStation(Location);
}

void APickUp::Server_PlaceInStation_Implementation(FVector Location)
{
	if (!bIsHeld) return;

	bIsHeld = false;
	HoldingCharacter = nullptr;
	HoldPointRef = nullptr;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	SetActorLocation(Location + FVector(0.f, 0.f, 20.f));
	ApplyDeliveredPhysics();

	SetItemState(EItemState::Processing);
}

// =============================================================================
// MarkReady
// =============================================================================

void APickUp::MarkReady()
{
	if (HasAuthority())
		Server_MarkReady_Implementation();
	else
		Server_MarkReady();
}

void APickUp::Server_MarkReady_Implementation()
{
	ApplyDropPhysics();
	SetItemState(EItemState::Ready);
}

// =============================================================================
// Multicast
// =============================================================================

void APickUp::Multicast_OnPickedUp_Implementation()
{
	BP_OnPickedUp();
}

void APickUp::Multicast_OnDropped_Implementation(bool bInDropZone)
{
	BP_OnDropped(bInDropZone);
}

// =============================================================================
// RepNotify
// =============================================================================

void APickUp::OnRep_ItemState()
{
	BP_OnStateChanged(ItemState);
	UpdateItemColor();
}

void APickUp::OnRep_IsHeld()
{
	UE_LOG(LogTemp, Warning, TEXT("[PICKUP] OnRep_IsHeld: %s bIsHeld=%d bWasDelivered=%d State=%d HoldingChar=%s"),
		*GetName(), bIsHeld, bWasDelivered, (int32)ItemState,
		HoldingCharacter ? *HoldingCharacter->GetName() : TEXT("NULL"));

	if (bIsHeld)
	{
		// Attach visual en el cliente observador
		if (HoldingCharacter && HoldingCharacter->GetHoldPoint())
		{
			AttachToComponent(
				HoldingCharacter->GetHoldPoint(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale
			);
		}
		ApplyPickupPhysics();
	}
	else
	{
		// FIX PRINCIPAL: siempre desattachar al soltar, sin esto el item
		// queda adjunto visualmente y la InteractionSphere permanece desactivada,
		// impidiendo que cualquier otro jugador lo recoja
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		if (!bWasDelivered
			&& ItemState != EItemState::Delivered
			&& ItemState != EItemState::Processing)
		{
			// Soltado al suelo — reactivar colision e InteractionSphere
			ApplyDropPhysics();
		}
		else
		{
			// Entregado o en station — sin colision ni fisica
			ApplyDeliveredPhysics();
		}
	}
}

void APickUp::OnRep_IsOnSpawner()
{
	if (bIsOnSpawner)
	{
		BobTime = 0.f;
	}
}

void APickUp::UpdateItemColor()
{
	if (!DynamicMaterial)
	{
		if (Mesh && Mesh->GetNumMaterials() > 0 && Mesh->GetMaterial(0))
		{
			DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
		}
	}
	if (!DynamicMaterial) return;

	FLinearColor NewColor;
	switch (ItemState)
	{
	case EItemState::Raw:        NewColor = FLinearColor(0.8f, 0.6f, 0.4f, 1.f); break;
	case EItemState::Processing: NewColor = FLinearColor(1.f, 0.85f, 0.f, 1.f);  break;
	case EItemState::Ready:      NewColor = FLinearColor(0.1f, 0.85f, 0.2f, 1.f); break;
	case EItemState::Delivered:  NewColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f); break;
	default:                     NewColor = FLinearColor::White; break;
	}

	DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), NewColor);
}
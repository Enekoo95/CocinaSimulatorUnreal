// Fill out your copyright notice in the Description page of Project Settings.

#include "PickUp.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"


APickUp::APickUp()
{
	PrimaryActorTick.bCanEverTick = true;

	// Replicación obligatoria para un objeto multijugador
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

	// El bobbing solo lo ejecuta el servidor (SetActorLocation replica el transform).
	// Los clientes solo ven el resultado replicado, no calculan la posición ellos mismos.
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
	DOREPLIFETIME(APickUp, bIsOnSpawner);   // <-- nuevo
}


void APickUp::PickUpItem(USceneComponent* HoldPoint)
{
	if (!HoldPoint || bIsHeld) return;

	if (HasAuthority())
	{
		// Ejecutamos directo si ya somos el servidor
		Server_PickUp_Implementation(HoldPoint);
	}
	else
	{
		// Cliente: pedimos al servidor que lo haga
		Server_PickUp(HoldPoint);
	}
}

void APickUp::DropItem(FVector DropLocation, bool bInDropZone)
{
	if (!bIsHeld) return;

	if (HasAuthority())
	{
		Server_Drop_Implementation(DropLocation, bInDropZone);
	}
	else
	{
		Server_Drop(DropLocation, bInDropZone);
	}
}

void APickUp::PlaceInStation(const FVector& Location)
{
	if (!bIsHeld) return;

	if (HasAuthority())
	{
		Server_PlaceInStation_Implementation(Location);
	}
	else
	{
		Server_PlaceInStation(Location);
	}
}

void APickUp::MarkReady()
{
	if (HasAuthority())
	{
		Server_MarkReady_Implementation();
	}
	else
	{
		Server_MarkReady();
	}
}

void APickUp::SetItemState(EItemState NewState)
{
	// Solo el servidor modifica el estado canónico
	if (!HasAuthority()) return;
	if (ItemState == NewState) return;

	ItemState = NewState;
	BP_OnStateChanged(ItemState); // servidor
	UpdateItemColor();            // servidor
	// OnRep_ItemState se dispara en clientes automáticamente
}


void APickUp::Server_PickUp_Implementation(USceneComponent* HoldPoint)
{
	if (!HoldPoint || bIsHeld) return;

	// Actualizar estado replicado
	bIsHeld = true;
	bIsOnSpawner = false;
	bWasDelivered = false;
	HoldPointRef = HoldPoint;

	// Física desactivada mientras se lleva
	SetReplicateMovement(false); // El attachment se encarga del transform
	Mesh->SetSimulatePhysics(false);
	SetActorEnableCollision(false);

	// Attachment se replica automáticamente en UE5
	AttachToComponent(HoldPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// Notificar a TODOS los clientes del evento visual/sonoro
	Multicast_OnPickedUp();
}

void APickUp::Server_Drop_Implementation(FVector DropLocation, bool bInDropZone)
{
	if (!bIsHeld) return;

	bIsHeld = false;
	HoldPointRef = nullptr;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (bInDropZone)
	{
		bWasDelivered = true;
		ItemState = EItemState::Delivered;

		SetActorLocation(DropLocation);
		SetActorEnableCollision(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetSimulatePhysics(false);
		UpdateItemColor();
	}
	else
	{
		SetActorLocation(DropLocation + FVector(0.f, 0.f, 15.f));
		SetActorEnableCollision(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetSimulatePhysics(true);
		SetReplicateMovement(true); // Vuelve a replicar física
	}

	Multicast_OnDropped(bInDropZone);
}

void APickUp::Server_PlaceInStation_Implementation(FVector Location)
{
	if (!bIsHeld) return;

	bIsHeld = false;
	HoldPointRef = nullptr;
	ItemState = EItemState::Processing;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	SetActorLocation(Location + FVector(0.f, 0.f, 20.f));
	SetActorEnableCollision(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);
	UpdateItemColor();

	// Notificar cambio de estado a clientes (OnRep_ItemState)
}

void APickUp::Server_MarkReady_Implementation()
{
	ItemState = EItemState::Ready;
	SetActorEnableCollision(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
	SetReplicateMovement(true);
	UpdateItemColor();
}


void APickUp::Multicast_OnPickedUp_Implementation()
{
	// Aquí va cualquier efecto visual/sonoro que todos deben ver
	BP_OnPickedUp();
}

void APickUp::Multicast_OnDropped_Implementation(bool bInDropZone)
{
	BP_OnDropped(bInDropZone);
}


void APickUp::OnRep_ItemState()
{
	BP_OnStateChanged(ItemState);
	UpdateItemColor();
}

void APickUp::OnRep_IsHeld()
{

	if (!bIsHeld)
	{
		Mesh->SetSimulatePhysics(true);
		SetActorEnableCollision(true);
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
	case EItemState::Raw:
		NewColor = FLinearColor(0.8f, 0.6f, 0.4f, 1.f);
		break;
	case EItemState::Processing:
		NewColor = FLinearColor(1.f, 0.85f, 0.f, 1.f);
		break;
	case EItemState::Ready:
		NewColor = FLinearColor(0.1f, 0.85f, 0.2f, 1.f);
		break;
	case EItemState::Delivered:
		NewColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
		break;
	default:
		NewColor = FLinearColor::White;
		break;
	}

	DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), NewColor);
}
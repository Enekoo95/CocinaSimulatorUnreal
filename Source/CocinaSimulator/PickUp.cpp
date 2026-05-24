// Fill out your copyright notice in the Description page of Project Settings.

#include "PickUp.h"
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

	// Attachment handles following the hold point on all clients automatically.
	// Only do manual bobbing when on the spawner.
	if (bIsOnSpawner && !bIsHeld)
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
}

void APickUp::PickUp(USceneComponent* HoldPoint)
{
	if (!HoldPoint || bIsHeld) return;

	bIsHeld = true;
	bIsOnSpawner = false;
	bWasDelivered = false;
	HoldPointRef = HoldPoint;

	Mesh->SetSimulatePhysics(false);
	SetActorEnableCollision(false);

	// Attach to the hold point so all clients see the object follow the character.
	// Actor attachment replicates automatically in Unreal.
	AttachToComponent(HoldPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	BP_OnPickedUp();
}

void APickUp::Drop(FVector DropLocation, bool bInDropZone)
{
	if (!bIsHeld && !bInDropZone) return;

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
	}

	BP_OnDropped(bInDropZone);
}

void APickUp::PlaceInStation(const FVector& Location)
{
	if (!bIsHeld)
	{
		return;
	}

	bIsHeld = false;
	HoldPointRef = nullptr;
	ItemState = EItemState::Processing;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	SetActorLocation(Location + FVector(0.f, 0.f, 20.f));
	SetActorEnableCollision(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);
	UpdateItemColor();
}

void APickUp::MarkReady()
{
	ItemState = EItemState::Ready;
	SetActorEnableCollision(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
	UpdateItemColor();
}

void APickUp::SetItemState(EItemState NewState)
{
	if (ItemState == NewState)
	{
		return;
	}

	ItemState = NewState;
	BP_OnStateChanged(ItemState);
	UpdateItemColor();
}

void APickUp::OnRep_ItemState()
{
	BP_OnStateChanged(ItemState);
	UpdateItemColor();
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

	if (!DynamicMaterial)
	{
		return;
	}

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
	}

	DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), NewColor);
}
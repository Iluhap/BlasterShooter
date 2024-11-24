// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpawnPoint.h"
#include "Pickups/Pickup.h"


APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MinSpawnRate = 5.f;
	MaxSpawnRate = 15.f;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	StartSpawnTimer();
}

void APickupSpawnPoint::SpawnPickup()
{
	if (PickupClasses.IsEmpty())
		return;

	const int32 Selection = FMath::RandRange(0, PickupClasses.Num() - 1);

	if (auto* SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection],
	                                                          GetActorLocation(),
	                                                          FRotator::ZeroRotator);
		IsValid(SpawnedPickup) and HasAuthority())
	{
		SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::OnPickupDestroyed);
	}
}

void APickupSpawnPoint::StartSpawnTimer()
{
	if (not HasAuthority())
		return;

	const float RandomSpawnRate = FMath::RandRange(MinSpawnRate, MaxSpawnRate);

	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle,
	                                       this, &APickupSpawnPoint::SpawnTimerFinished,
	                                       RandomSpawnRate);
}

void APickupSpawnPoint::SpawnTimerFinished()
{
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

void APickupSpawnPoint::OnPickupDestroyed(AActor* DestroyedActor)
{
	StartSpawnTimer();
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();

protected:
	virtual void BeginPlay() override;

public:
	void SpawnPickup();

private:
	void StartSpawnTimer();
	void SpawnTimerFinished();

	UFUNCTION()
	void OnPickupDestroyed(AActor* DestroyedActor);

private:
	UPROPERTY(EditAnywhere, Category=Spawn)
	TArray<TSubclassOf<class APickup>> PickupClasses;

	UPROPERTY(EditAnywhere, Category=Spawn)
	float MinSpawnRate;

	UPROPERTY(EditAnywhere, Category=Spawn)
	float MaxSpawnRate;

	FTimerHandle SpawnTimerHandle;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()

public:
	APickup();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                          const FHitResult& SweepResult);

public:
	virtual void Tick(float DeltaTime) override;

public:
	virtual void Destroyed() override;

private:
	void BindOverlapTimerFinished();
	
private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<class USphereComponent> OverlapSphere;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UNiagaraComponent> PickupEffectComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UNiagaraSystem> PickupEffect;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> PickupSound;

	UPROPERTY(EditAnywhere)
	float BaseTurnRate;

	UPROPERTY(EditAnywhere)
	float BindOverlapDelay;
	
	FTimerHandle BindOverlapTimer;
};

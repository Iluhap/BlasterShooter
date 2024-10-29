// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundCue.h"
#include "AmmoCasing.generated.h"


UCLASS()
class BLASTER_API AAmmoCasing : public AActor
{
	GENERATED_BODY()

public:
	AAmmoCasing();
	
protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void PerformDestroy();

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent,
					   AActor* OtherActor, UPrimitiveComponent* OtherComp,
					   FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere)
	float EjectionImpulse;
	
	UPROPERTY(EditAnywhere)
	float DestroyDelay;

	FTimerHandle DestroyTimerHandle;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> HitSound;
};

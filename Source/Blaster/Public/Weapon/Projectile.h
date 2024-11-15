// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundCue.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent,
	                   AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);

	virtual void PlayImpactEffects() const;

	void StartDestroyTimer();
	void ClearDestroyTimer();
	void DestroyTimerFinished();

	void ExplodeDamage();

public:
	virtual void Tick(float DeltaTime) override;

protected:
	void SpawnTrailSystem();
	void HideProjectile();

protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UProjectileMovementComponent> MovementComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UBoxComponent> CollisionBox;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UNiagaraSystem> TrailSystem;

	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> TrailSystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(EditAnywhere, Category=Damage)
	float Damage;

private:
	UPROPERTY(EditAnywhere, Category=Damage)
	float RadialDamageInnerRadius;

	UPROPERTY(EditAnywhere, Category=Damage)
	float RadialDamageOuterRadius;

private:
	FTimerHandle DestroyTimerHandle;

	UPROPERTY(EditAnywhere)
	float DestroyDelay;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer;

	TObjectPtr<UParticleSystemComponent> TracerComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticles;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ImpactSound;
};

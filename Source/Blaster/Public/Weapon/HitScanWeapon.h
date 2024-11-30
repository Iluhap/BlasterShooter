// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	AHitScanWeapon();

public:
	virtual void Fire(const FVector& HitTarget) override;

public:
	virtual void ServerFire_Implementation(const FVector_NetQuantize& Start, const FVector_NetQuantize& HitTarget) override;
	virtual void NetMulticastFire_Implementation(const FVector_NetQuantize& HitTarget) override;

protected:
	bool TraceHit(const FVector& Start, const FVector& HitTarget, FHitResult& HitResult);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastSpawnImpactEffects(const FHitResult& HitResult);

protected:
	void SpawnImpactParticles(const FHitResult& HitResult) const;
	void SpawnBeamParticles(const FTransform& StartTransform, const FVector& BeamEnd) const;
	void SpawnMuzzleFlashEffects(const FTransform& MuzzleTransform) const;
	void SpawnHitSound(const FVector& HitLocation) const;

	virtual TOptional<FHitResult> PerformHitScan(const FVector& Start, const FVector& End);

	void ApplyDamage(AActor* DamagedActor) const;

private:
	UPROPERTY(EditAnywhere)
	float Damage;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticles;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> BeamParticles;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> MuzzleFlash;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> FireSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> HitSound;
};

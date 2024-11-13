﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

private:
	void SpawnImpactParticles(const FHitResult& HitResult) const;
	void SpawnBeamParticles(const FTransform& StartTransform, const FVector& BeamEnd) const;
	void SpawnMuzzleFlashEffects(const FTransform& MuzzleTransform) const;
	void SpawnHitSound(const FVector& HitLocation) const;

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

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	AProjectileWeapon();

public:
	virtual void Fire(const FVector& HitTarget) override;

public:
	virtual void ServerFire_Implementation(const FVector_NetQuantize& HitTarget) override;

private:
	class AProjectile* SpawnProjectile(const TSubclassOf<AProjectile>& Class,
	                                   const FVector_NetQuantize& HitTarget) const;

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"


AProjectileWeapon::AProjectileWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AProjectileWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectileWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
}

void AProjectileWeapon::ServerFire_Implementation(const FVector_NetQuantize& Start,
                                                  const FVector_NetQuantize& HitTarget)
{
	const FVector DirectionToTarget = HitTarget - Start;

	if (IsValid(ProjectileClass))
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());

		GetWorld()->SpawnActor<AProjectile>(
			ProjectileClass,
			Start,
			DirectionToTarget.Rotation(),
			Params
		);
	}

	Super::ServerFire_Implementation(Start, HitTarget);
}

void AProjectileWeapon::NetMulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	Super::NetMulticastFire_Implementation(HitTarget);
}

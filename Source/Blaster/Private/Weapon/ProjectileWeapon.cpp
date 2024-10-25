// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"


AProjectileWeapon::AProjectileWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (not HasAuthority())
		return;

	auto* InstigatorPawn = Cast<APawn>(GetOwner());

	if (const auto* MuzzleFlashSocket = Mesh->GetSocketByName(FName { "MuzzleFlash" });
		IsValid(MuzzleFlashSocket))
	{
		const auto SocketTransform = MuzzleFlashSocket->GetSocketTransform(Mesh);

		const FVector DirectionToTarget = HitTarget - SocketTransform.GetLocation();

		if (IsValid(ProjectileClass) and IsValid(InstigatorPawn))
		{
			FActorSpawnParameters Params;
			Params.Owner = GetOwner();
			Params.Instigator = InstigatorPawn;

			GetWorld()->SpawnActor<AProjectile>(
				ProjectileClass,
				SocketTransform.GetLocation(),
				DirectionToTarget.Rotation(),
				Params
			);
		}
	}
}

void AProjectileWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectileWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

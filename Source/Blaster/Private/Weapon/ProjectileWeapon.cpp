// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"

#include "Character/BlasterCharacter.h"
#include "Weapon/Projectile.h"


AProjectileWeapon::AProjectileWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (bUseServerSideRewind and IsValid(OwningBlasterCharacter))
	{
		if (OwningBlasterCharacter->HasAuthority())
			return;

		if (OwningBlasterCharacter->IsLocallyControlled())
		{
			if (const auto& Muzzle = GetMuzzleTransform();
				Muzzle.IsSet())
			{
				auto* SpawnedProjectile = SpawnProjectile(ServerSideRewindProjectileClass, HitTarget);

				SpawnedProjectile->bUseServerSideRewind = true;
				SpawnedProjectile->TraceStart = Muzzle->GetLocation();
				SpawnedProjectile->InitialVelocity =
					SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				SpawnedProjectile->Damage = Damage;
			}
		}
		else
		{
			auto* SpawnedProjectile = SpawnProjectile(ServerSideRewindProjectileClass, HitTarget);
			SpawnedProjectile->bUseServerSideRewind = false;
		}
	}
}

void AProjectileWeapon::ServerFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	Super::ServerFire_Implementation(HitTarget);

	AProjectile* SpawnedProjectile = nullptr;

	if (bUseServerSideRewind)
	{
		if (const auto* InstigatorPawn = Cast<APawn>(GetOwner());
			IsValid(InstigatorPawn) and InstigatorPawn->IsLocallyControlled())
		{
			SpawnedProjectile = SpawnProjectile(ProjectileClass, HitTarget);
			SpawnedProjectile->bUseServerSideRewind = false;
			SpawnedProjectile->Damage = Damage;
		}
		else
		{
			SpawnedProjectile = SpawnProjectile(ServerSideRewindProjectileClass, HitTarget);
			SpawnedProjectile->bUseServerSideRewind = true;
		}
	}
	else
	{
		if (IsValid(ProjectileClass))
		{
			SpawnedProjectile = SpawnProjectile(ProjectileClass, HitTarget);
			SpawnedProjectile->bUseServerSideRewind = false;
			SpawnedProjectile->Damage = Damage;
		}
	}
}

AProjectile* AProjectileWeapon::SpawnProjectile(const TSubclassOf<AProjectile>& Class,
                                                const FVector_NetQuantize& HitTarget) const
{
	auto* InstigatorPawn = Cast<APawn>(GetOwner());

	if (not IsValid(InstigatorPawn))
		return nullptr;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = InstigatorPawn;

	if (auto Muzzle = GetMuzzleTransform();
		Muzzle.IsSet())
	{
		const FVector Start = Muzzle->GetLocation();
		const FVector DirectionToTarget = HitTarget - Start;

		return GetWorld()->SpawnActor<AProjectile>(Class,
		                                           Start, DirectionToTarget.Rotation(),
		                                           Params);
	}

	return nullptr;
}

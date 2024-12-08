﻿#include "Weapon/Shotgun.h"


AShotgun::AShotgun()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AShotgun::BeginPlay()
{
	Super::BeginPlay();
}

void AShotgun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AShotgun::Fire(const FVector& HitTarget)
{
	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		const FVector Start = MuzzleTransform->GetLocation();

		TArray<FVector_NetQuantize> HitTargets;
		HitTargets.SetNumZeroed(NumberOfPellets);

		for (auto& HitPoint : HitTargets)
		{
			HitPoint = ApplyScatterTo(Start, HitTarget);
			SpawnBeamParticles(MuzzleTransform.GetValue(), HitPoint);
		}

		LocalFirePellets(HitTargets);
		ServerFirePellets(HitTargets);
	}
}

void AShotgun::LocalFirePellets(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::LocalFire({});

	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		for (auto& HitPoint : HitTargets)
		{
			SpawnBeamParticles(MuzzleTransform.GetValue(), HitPoint);
		}
	}
}

void AShotgun::ServerFirePellets_Implementation(const TArray<FVector_NetQuantize>& HitTargets)
{
	SpendRound();
	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		TArray<FHitResult> HitResults;
		for (auto& HitTarget : HitTargets)
		{
			if (FHitResult HitResult;
				TraceHit(MuzzleTransform->GetLocation(), HitTarget, HitResult))
			{
				ApplyDamage(HitResult.GetActor());
				HitResults.Add(HitResult);
			}
			else
			{
				FHitResult MockHitResult;
				MockHitResult.ImpactPoint = HitTarget;
				MockHitResult.bBlockingHit = false;
				HitResults.Add(MockHitResult);
			}
		}
		NetMulticastFirePellets(HitResults);
	}
}

void AShotgun::NetMulticastFirePellets_Implementation(const TArray<FHitResult>& HitResults)
{
	for (const auto& HitResult : HitResults)
	{
		SpawnFireEffects(HitResult);
	}
}

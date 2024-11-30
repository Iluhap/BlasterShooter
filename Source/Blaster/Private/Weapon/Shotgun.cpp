#include "Weapon/Shotgun.h"


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
		ServerFire(MuzzleTransform->GetLocation(), HitTarget);
	}
}

void AShotgun::ServerFire_Implementation(const FVector_NetQuantize& Start, const FVector_NetQuantize& HitTarget)
{
	SpendRound();

	for (int32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector End = ApplyScatterTo(Start, HitTarget);
		if (const auto HitResult = PerformHitScan(Start, End);
			HitResult.IsSet())
		{
			NetMulticastSpawnImpactEffects(HitResult.GetValue());
		}
		else
		{
			NetMulticastFire(End);
		}
	}
}

void AShotgun::NetMulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	Super::NetMulticastFire_Implementation(HitTarget);
}

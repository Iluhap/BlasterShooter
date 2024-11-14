#include "Weapon/Shotgun.h"

#include "Engine/SkeletalMeshSocket.h"


AShotgun::AShotgun()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	if (auto* MuzzleFlashSocket = Mesh->GetSocketByName("MuzzleFlash");
		IsValid(MuzzleFlashSocket))
	{
		const FTransform MuzzleTransform = MuzzleFlashSocket->GetSocketTransform(Mesh);
		const FVector Start = MuzzleTransform.GetLocation();

		for (int32 i = 0; i < NumberOfPellets; i++)
		{
			if (FHitResult HitResult;
				TraceHit(Start, HitTarget, HitResult))
			{
				ApplyDamage(HitResult.GetActor());
				SpawnImpactParticles(HitResult);
				SpawnHitSound(HitResult.ImpactPoint);
			}
		}
	}
}

void AShotgun::BeginPlay()
{
	Super::BeginPlay();
}

void AShotgun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#include "Weapon/Shotgun.h"

#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Components/LagCompensationComponent.h"


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
		}

		LocalFirePellets(HitTargets);
	}
}

void AShotgun::LocalFirePellets(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::LocalFire({});

	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		FShotgunServerSideRewindRequest Request;
		for (auto& HitPoint : HitTargets)
		{
			SpawnBeamParticles(MuzzleTransform.GetValue(), HitPoint);
			if (FHitResult HitResult;
				TraceHit(MuzzleTransform->GetLocation(), HitPoint, HitResult))
			{
				SpawnFireEffects(HitResult);
				if (auto* BlasterCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
					IsValid(BlasterCharacter))
				{
					Request.Requests.Add({
						.HitCharacter = BlasterCharacter,
						.TraceStart = MuzzleTransform->GetLocation(),
						.HitLocation = HitResult.ImpactPoint,
						.HitTime = OwningBlasterPlayerController->GetServerTime()
					});
				}
			}
		}
		ServerConfirmPelletsHit(Request);
	}
}

void AShotgun::ServerConfirmPelletsHit_Implementation(const FShotgunServerSideRewindRequest& Request)
{
	if (not IsValid(OwningBlasterCharacter))
		return;

	if (auto* LagCompensationComponent = OwningBlasterCharacter->FindComponentByClass<ULagCompensationComponent>();
		IsValid(LagCompensationComponent))
	{
		if (const auto& [TracedCharacters] = LagCompensationComponent->ShotgunServerSideRewind(Request);
			not TracedCharacters.IsEmpty())
		{
			for (const auto& [Character, Results] : TracedCharacters)
			{
				for (const auto& [TracedHitBoxes] : Results.Results)
				{
					ApplyDamage(Character);
					for (const auto& [Name, HitResult] : TracedHitBoxes)
						NetMulticastSpawnFireEffects(HitResult);
				}
			}
		}
	}
}

void AShotgun::NetMulticastFirePellets_Implementation(const TArray<FHitResult>& HitResults)
{
	for (const auto& HitResult : HitResults)
	{
		SpawnFireEffects(HitResult);
	}
}

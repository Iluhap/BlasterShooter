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
		FShotgunRewindRequest Request;
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
						.Base = {
							.HitCharacter = BlasterCharacter,
							.TraceStart = MuzzleTransform->GetLocation(),
							.HitTime = OwningBlasterPlayerController->GetServerTime()
						},
						.HitLocation = HitResult.ImpactPoint,
					});
				}
			}
		}
		ServerConfirmPelletsHit(Request);
	}
}

void AShotgun::OnEquipped()
{
	Super::OnEquipped();
	SubscribeOnShotgunHitConfirmed();
}

void AShotgun::OnEquippedSecondary()
{
	Super::OnEquippedSecondary();
	UnsubscribeOnHitConfirmed();
}

void AShotgun::OnDropped()
{
	Super::OnDropped();
	UnsubscribeOnHitConfirmed();
}

void AShotgun::ServerConfirmPelletsHit_Implementation(const FShotgunRewindRequest& Request)
{
	if (auto* LagCompensationComponent = GetLagCompensationComponent())
	{
		LagCompensationComponent->ServerConfirmShotgunHit(Request);
	}
}

void AShotgun::OnShotgunHitConfirmed(const FShotgunRewindResult& Result)
{
	if (const auto& [TracedCharacters] = Result;
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

void AShotgun::SubscribeOnShotgunHitConfirmed()
{
	if (HasAuthority())
	{
		if (auto* LagCompensation = GetLagCompensationComponent();
			IsValid(LagCompensation))
		{
			LagCompensation->OnShotgunHitConfirmed.AddUniqueDynamic(this, &AShotgun::OnShotgunHitConfirmed);
		}
	}
}

void AShotgun::UnsubscribeOnShotgunHitConfirmed()
{
	if (HasAuthority())
	{
		if (auto* LagCompensation = GetLagCompensationComponent();
			IsValid(LagCompensation))
		{
			LagCompensation->OnShotgunHitConfirmed.RemoveDynamic(this, &AShotgun::OnShotgunHitConfirmed);
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

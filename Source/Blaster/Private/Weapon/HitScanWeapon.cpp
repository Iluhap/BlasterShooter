// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Components/LagCompensationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

AHitScanWeapon::AHitScanWeapon()
{
	DistanceToSphere = 800.f;
	SphereRadius = 75.f;
	bUseScatter = true;

	Damage = 10.f;

	bUseServerSideRewind = true;
}

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
}

void AHitScanWeapon::LocalFire(const FVector& HitTarget)
{
	Super::LocalFire(HitTarget);

	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		SpawnMuzzleFlashEffects(MuzzleTransform.GetValue());
		SpawnBeamParticles(MuzzleTransform.GetValue(), HitTarget);

		const FVector Start = MuzzleTransform->GetLocation();

		if (FHitResult HitResult;
			TraceHit(Start, HitTarget, HitResult))
		{
			SpawnFireEffects(HitResult);
			if (auto* BlasterCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
				IsValid(BlasterCharacter))
			{
				ServerConfirmHit({
					.Base = {
						.HitCharacter = BlasterCharacter,
						.TraceStart = Start,
						.HitTime = OwningBlasterPlayerController->GetServerTime()
					},
					.HitLocation = HitResult.ImpactPoint,
				});
			}
		}
	}
}

void AHitScanWeapon::ServerConfirmHit_Implementation(const FHitScanRewindRequest& Request)
{
	if (auto* LagCompensationComponent = GetLagCompensationComponent())
	{
		LagCompensationComponent->ServerConfirmHitScan(Request);
	}

	FHitResult MockHitResult;
	MockHitResult.ImpactPoint = Request.HitLocation;
	MockHitResult.bBlockingHit = false;
	NetMulticastSpawnFireEffects(MockHitResult);
}

void AHitScanWeapon::OnHitConfirmed(ABlasterCharacter* HitCharacter, const FRewindResult& Result)
{
	Super::OnHitConfirmed(HitCharacter, Result);

	if (const auto& [TracedHitBoxes] = Result;
		not TracedHitBoxes.IsEmpty())
	{
		for (const auto& [Name, HitResult] : TracedHitBoxes)
		{
			NetMulticastSpawnFireEffects(HitResult);
		}
	}
}

void AHitScanWeapon::NetMulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	Super::NetMulticastFire_Implementation(HitTarget);

	if (IsValid(OwningBlasterCharacter) and not OwningBlasterCharacter->IsLocallyControlled())
	{
		if (const auto MuzzleTransform = GetMuzzleTransform();
			MuzzleTransform.IsSet())
		{
			SpawnMuzzleFlashEffects(MuzzleTransform.GetValue());
		}
	}
}

bool AHitScanWeapon::TraceHit(const FVector& Start, const FVector& HitTarget, FHitResult& HitResult) const
{
	const FVector End = Start + (HitTarget - Start) * 1.25f;

	return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
}

void AHitScanWeapon::NetMulticastSpawnFireEffects_Implementation(const FHitResult& HitResult)
{
	if (IsValid(OwningBlasterCharacter) and not OwningBlasterCharacter->IsLocallyControlled())
	{
		SpawnFireEffects(HitResult);
	}
}

void AHitScanWeapon::SpawnImpactParticles(const FVector& ImpactPoint) const
{
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticles,
			ImpactPoint,
			FRotator::ZeroRotator
		);
	}
}

void AHitScanWeapon::SpawnBeamParticles(const FTransform& StartTransform, const FVector& BeamEnd) const
{
	if (IsValid(BeamParticles))
	{
		if (auto* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(), BeamParticles, StartTransform);
			IsValid(Beam))
		{
			Beam->SetVectorParameter(FName("Target"), BeamEnd);
		}
	}
}

void AHitScanWeapon::SpawnMuzzleFlashEffects(const FTransform& MuzzleTransform) const
{
	if (IsValid(MuzzleFlash))
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			MuzzleFlash,
			MuzzleTransform);
	}
	if (IsValid(FireSound))
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			FireSound,
			MuzzleTransform.GetLocation());
	}
}

void AHitScanWeapon::SpawnHitSound(const FVector& HitLocation) const
{
	if (IsValid(HitSound))
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			HitSound,
			HitLocation,
			FRotator::ZeroRotator,
			0.5f,
			FMath::FRandRange(-0.5f, 0.5f));
	}
}

void AHitScanWeapon::SpawnFireEffects(const FHitResult& HitResult) const
{
	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		SpawnBeamParticles(MuzzleTransform.GetValue(), HitResult.ImpactPoint);
	}

	if (HitResult.bBlockingHit)
	{
		SpawnImpactParticles(HitResult.ImpactPoint);
		SpawnHitSound(HitResult.ImpactPoint);
	}
}

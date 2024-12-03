// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

AHitScanWeapon::AHitScanWeapon()
{
	Damage = 10;

	DistanceToSphere = 800.f;
	SphereRadius = 75.f;
	bUseScatter = true;
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
		SpawnBeamParticles(MuzzleTransform.GetValue(), HitTarget);
	}
}

void AHitScanWeapon::ServerFire_Implementation(const FVector_NetQuantize& Start, const FVector_NetQuantize& HitTarget)
{
	FVector End = HitTarget;
	if (const auto HitResult = PerformHitScan(Start, HitTarget);
		HitResult.IsSet())
	{
		End = HitResult->ImpactPoint;
		NetMulticastSpawnFireEffects(HitResult.GetValue());
	}

	Super::ServerFire_Implementation(Start, End);
}

void AHitScanWeapon::NetMulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	Super::NetMulticastFire_Implementation(HitTarget);

	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		SpawnMuzzleFlashEffects(MuzzleTransform.GetValue());
	}
}

bool AHitScanWeapon::TraceHit(const FVector& Start, const FVector& HitTarget, FHitResult& HitResult)
{
	const FVector End = Start + (HitTarget - Start) * 1.25f;

	return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
}

void AHitScanWeapon::NetMulticastSpawnFireEffects_Implementation(const FHitResult& HitResult)
{
	SpawnFireEffects(HitResult);
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
	if (IsValid(OwningBlasterCharacter)
	and not OwningBlasterCharacter->IsLocallyControlled())
	{
		if (const auto MuzzleTransform = GetMuzzleTransform();
			MuzzleTransform.IsSet())
		{
			SpawnBeamParticles(MuzzleTransform.GetValue(), HitResult.ImpactPoint);
		}
	}

	if (HitResult.bBlockingHit)
	{
		SpawnImpactParticles(HitResult.ImpactPoint);
		SpawnHitSound(HitResult.ImpactPoint);
	}
}

TOptional<FHitResult> AHitScanWeapon::PerformHitScan(const FVector& Start, const FVector& End)
{
	if (FHitResult HitResult;
		TraceHit(Start, End, HitResult))
	{
		if (HitResult.bBlockingHit)
		{
			ApplyDamage(HitResult.GetActor());

			return HitResult;
		}
	}
	return {};
}

void AHitScanWeapon::ApplyDamage(AActor* DamagedActor) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (not IsValid(OwnerPawn))
		return;

	AController* InstigatorController = OwnerPawn->GetController();
	if (not(IsValid(InstigatorController) and HasAuthority()))
		return;

	if (auto* BlasterCharacter = Cast<ABlasterCharacter>(DamagedActor);
		IsValid(BlasterCharacter))
	{
		UGameplayStatics::ApplyDamage(
			BlasterCharacter,
			Damage,
			InstigatorController,
			GetOwner(),
			UDamageType::StaticClass());
	}
}

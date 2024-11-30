// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "ParticleHelper.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
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

void AHitScanWeapon::ServerFire_Implementation(const FVector_NetQuantize& Start, const FVector_NetQuantize& HitTarget)
{
	if (const auto HitResult = PerformHitScan(Start, HitTarget);
		HitResult.IsSet())
	{
		Super::ServerFire_Implementation(Start, HitResult.GetValue().ImpactPoint);
	}

	Super::ServerFire_Implementation(Start, HitTarget);
}

void AHitScanWeapon::NetMulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	Super::NetMulticastFire_Implementation(HitTarget);

	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		SpawnBeamParticles(MuzzleTransform.GetValue(), HitTarget);
		SpawnMuzzleFlashEffects(MuzzleTransform.GetValue());
	}
}

bool AHitScanWeapon::TraceHit(const FVector& Start, const FVector& HitTarget, FHitResult& HitResult)
{
	const FVector End = Start + (HitTarget - Start) * 1.25f;

	return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
}

void AHitScanWeapon::NetMulticastSpawnImpactEffects_Implementation(const FHitResult& HitResult)
{
	if (const auto MuzzleTransform = GetMuzzleTransform();
		MuzzleTransform.IsSet())
	{
		SpawnBeamParticles(MuzzleTransform.GetValue(), HitResult.ImpactPoint);
	}

	SpawnImpactParticles(HitResult);
	SpawnHitSound(HitResult.ImpactPoint);
}

void AHitScanWeapon::SpawnImpactParticles(const FHitResult& HitResult) const
{
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticles,
			HitResult.ImpactPoint,
			HitResult.ImpactNormal.Rotation()
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

TOptional<FHitResult> AHitScanWeapon::PerformHitScan(const FVector& Start, const FVector& End)
{
	if (FHitResult HitResult;
		TraceHit(Start, End, HitResult))
	{
		if (HitResult.bBlockingHit)
		{
			ApplyDamage(HitResult.GetActor());
			NetMulticastSpawnImpactEffects(HitResult);

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

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

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
	bUserScatter = true;
}

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (auto* MuzzleFlashSocket = Mesh->GetSocketByName("MuzzleFlash");
		IsValid(MuzzleFlashSocket))
	{
		const FTransform MuzzleTransform = MuzzleFlashSocket->GetSocketTransform(Mesh);
		const FVector Start = MuzzleTransform.GetLocation();

		FVector BeamEnd = HitTarget;

		if (FHitResult HitResult;
			TraceHit(Start, HitTarget, HitResult))
		{
			if (HitResult.bBlockingHit)
			{
				ApplyDamage(HitResult.GetActor());

				BeamEnd = HitResult.ImpactPoint;

				SpawnImpactParticles(HitResult);
				SpawnHitSound(BeamEnd);
			}
		}
		SpawnBeamParticles(MuzzleTransform, BeamEnd);
		SpawnMuzzleFlashEffects(MuzzleTransform);
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	const FVector RandomDirection = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLocation = SphereCenter + RandomDirection;
	const FVector ToEndLocation = (EndLocation - TraceStart).GetSafeNormal();
	const FVector TraceEnd = TraceStart + ToEndLocation * (FVector::Distance(TraceStart, HitTarget) + 100.f);

	/*
	
	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 10, FColor::Cyan, false, 2);
	DrawDebugSphere(GetWorld(), EndLocation, 3, 10, FColor::Green, false, 2);
	DrawDebugSphere(GetWorld(), HitTarget, 10, 10, FColor::Red, false, 2);
	DrawDebugLine(GetWorld(), TraceStart, EndLocation, FColor::Yellow, false, 2.f);
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 2.f);

	*/

	return TraceEnd;
}

bool AHitScanWeapon::TraceHit(const FVector& Start, const FVector& HitTarget, FHitResult& HitResult)
{
	const FVector End = bUserScatter ? TraceEndWithScatter(Start, HitTarget) : HitTarget;
	FVector BeamEnd = End;

	bool HasHit;

	if (HasHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
		HasHit)
	{
		if (HitResult.bBlockingHit)
		{
			BeamEnd = HitResult.ImpactPoint;
		}
	}
	SpawnBeamParticles(FTransform { Start }, BeamEnd);
	return HasHit;
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

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (auto* MuzzleFlashSocket = Mesh->GetSocketByName("MuzzleFlash");
		IsValid(MuzzleFlashSocket))
	{
		const FTransform MuzzleTransform = MuzzleFlashSocket->GetSocketTransform(Mesh);
		const FVector Start = MuzzleTransform.GetLocation();
		const FVector End = Start + (HitTarget - Start) * 1.25f;

		if (FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility))
		{
			FVector BeamEnd = End;
			if (HitResult.bBlockingHit)
			{
				ApplyDamage(HitResult.GetActor());

				SpawnImpactParticles(HitResult);

				BeamEnd = HitResult.ImpactPoint;
			}

			SpawnBeamParticles(MuzzleTransform, BeamEnd);
		}
	}
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

void AHitScanWeapon::ApplyDamage(AActor* DamagedActor) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (not IsValid(OwnerPawn))
		return;

	AController* InstigatorController = OwnerPawn->GetController();
	if (not IsValid(InstigatorController))
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

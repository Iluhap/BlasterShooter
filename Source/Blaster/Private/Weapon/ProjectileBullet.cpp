// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"

#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Components/LagCompensationComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


AProjectileBullet::AProjectileBullet()
{
	PrimaryActorTick.bCanEverTick = true;

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("Movement Component");
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->SetIsReplicated(true);
	MovementComponent->InitialSpeed = InitialSpeed;
	MovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR

void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.Property->IsValidLowLevel()
		                     ? PropertyChangedEvent.Property->GetFName()
		                     : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (IsValid(MovementComponent))
		{
			MovementComponent->InitialSpeed = InitialSpeed;
			MovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}

#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent,
                              AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
		IsValid(HitCharacter)
		and IsValid(OwnerCharacter) and IsValid(OwnerController))
	{
		if (OwnerCharacter->HasAuthority() and not bUseServerSideRewind)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
			Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
			return;
		}
		if (bUseServerSideRewind and OwnerCharacter->IsLocallyControlled())
		{
			FProjectileRewindRequest Request = {
				.Base = {
					.HitCharacter = HitCharacter,
					.TraceStart = TraceStart,
					.HitTime = OwnerController->GetServerTime() - OwnerController->GetNetTripTime()
				},
				.InitialVelocity = InitialVelocity
			};

			ConfirmHit(Request);
		}
	}

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileBullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

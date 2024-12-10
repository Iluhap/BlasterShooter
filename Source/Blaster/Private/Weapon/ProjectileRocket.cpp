// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Weapon/RocketMovementComponent.h"


AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("Rocket Mesh");
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>("Movement Component");
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->InitialSpeed = InitialSpeed;
	RocketMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR

void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.Property->IsValidLowLevel()
							 ? PropertyChangedEvent.Property->GetFName()
							 : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, InitialSpeed))
	{
		if (IsValid(MovementComponent))
		{
			MovementComponent->InitialSpeed = InitialSpeed;
			MovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}

#endif

void AProjectileRocket::Destroyed()
{
	// No Super::Destroyed() call in order to prevent executing PlayImpactEffects() twice
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	SpawnTrailSystem();

	if (IsValid(ProjectileLoop) and IsValid(LoopingSoundAttenuation))
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName {},
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f, 1.f, 0.f,
			LoopingSoundAttenuation,
			nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent,
                              AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
		return;

	ExplodeDamage();
	PlayImpactEffects();
	StartDestroyTimer();
	HideProjectile();

	if (IsValid(ProjectileLoopComponent) and ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

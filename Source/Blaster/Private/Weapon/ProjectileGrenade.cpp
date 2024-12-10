// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileGrenade.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AProjectileGrenade::AProjectileGrenade()
{
	PrimaryActorTick.bCanEverTick = true;

	bExplodeOnHit = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("Projectile Mesh");
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("Movement Component");
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->SetIsReplicated(true);
	MovementComponent->bShouldBounce = true;
}

#if WITH_EDITOR

void AProjectileGrenade::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.Property->IsValidLowLevel()
							 ? PropertyChangedEvent.Property->GetFName()
							 : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileGrenade, InitialSpeed))
	{
		if (IsValid(MovementComponent))
		{
			MovementComponent->InitialSpeed = InitialSpeed;
			MovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}

#endif

void AProjectileGrenade::Destroyed()
{
	ExplodeDamage();

	Super::Destroyed();
}

void AProjectileGrenade::BeginPlay()
{
	Super::BeginPlay();

	SpawnTrailSystem();
	StartDestroyTimer();

	MovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (IsValid(BounceSound))
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation(),
			FRotator::ZeroRotator);
	}
}

void AProjectileGrenade::OnHit(UPrimitiveComponent* HitComponent,
                               AActor* OtherActor, UPrimitiveComponent* OtherComp,
                               FVector NormalImpulse, const FHitResult& Hit)
{
	if (not bExplodeOnHit)
		return;

	if (const auto* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
		IsValid(BlasterCharacter) and OtherActor != GetOwner())
	{
		HideProjectile();
		ClearDestroyTimer();
		Destroy();
	}
}

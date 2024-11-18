// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/Blaster.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"


AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>("Collision Box");
	SetRootComponent(CollisionBox);

	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);

	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	Damage = 20.f;
	DestroyDelay = 3.f;

	RadialDamageInnerRadius = 200.f;
	RadialDamageOuterRadius = 500.f;
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	PlayImpactEffects();
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Tracer))
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer, CollisionBox,
			FName {}, GetActorLocation(), GetActorRotation(),
			EAttachLocation::KeepWorldPosition);
	}

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}

	if (auto* Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
		IsValid(Mesh))
	{
		CollisionBox->IgnoreComponentWhenMoving(Mesh, true);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent,
                        AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}

void AProjectile::PlayImpactEffects() const
{
	if (IsValid(ImpactParticles))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorld()->GetTimerManager().SetTimer(DestroyTimerHandle,
	                                       this, &AProjectile::DestroyTimerFinished,
	                                       DestroyDelay);
}

void AProjectile::ClearDestroyTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(DestroyTimerHandle);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::ExplodeDamage()
{
	if (const auto* FiringPawn = GetInstigator();
		IsValid(FiringPawn))
	{
		if (auto* FiringController = FiringPawn->GetController();
			IsValid(FiringController))
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				Damage * 0.1f,
				GetActorLocation(),
				RadialDamageInnerRadius, RadialDamageOuterRadius, 1.f,
				UDamageType::StaticClass(),
				{},
				this,
				FiringController
			);
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::SpawnTrailSystem()
{
	if (IsValid(TrailSystem))
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName {},
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void AProjectile::HideProjectile()
{
	if (IsValid(ProjectileMesh))
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (IsValid(CollisionBox))
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (IsValid(TrailSystemComponent)
		and TrailSystemComponent->GetSystemInstanceController().IsValid())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/AudioComponent.h"


AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>("Rocket Mesh");
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DestroyDelay = 3.f;
}

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
				200.f, 500.f, 1.f,
				UDamageType::StaticClass(),
				{},
				this,
				FiringController
			);
		}
	}

	PlayImpactEffects();

	GetWorld()->GetTimerManager().SetTimer(DestroyTimerHandle,
	                                       this, &AProjectileRocket::DestroyTimerFinished,
	                                       DestroyDelay);

	if (IsValid(RocketMesh))
	{
		RocketMesh->SetVisibility(false);
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

	if (IsValid(ProjectileLoopComponent) and ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>("Rocket Mesh");
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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


	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

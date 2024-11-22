// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/ShieldPickup.h"

#include "Components/HealthComponent.h"

AShieldPickup::AShieldPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	ShieldAmount = 100.f;
}

void AShieldPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                         AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                         bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent,
	                            OtherActor, OtherComp,
	                            OtherBodyIndex, bFromSweep, SweepResult);

	if (auto* Health = OtherActor->FindComponentByClass<UHealthComponent>();
		IsValid(Health))
	{
		Health->RestoreShield(ShieldAmount);
	}

	Destroy();
}

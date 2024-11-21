// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/HealthPickup.h"

#include "Components/HealthComponent.h"

AHealthPickup::AHealthPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	HealAmount = 100.f;
	HealingTime = 3.f;
}

void AHealthPickup::Destroyed()
{
	Super::Destroyed();
}

void AHealthPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                         AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                         int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent,
	                            OtherActor, OtherComp,
	                            OtherBodyIndex, bFromSweep, SweepResult);

	if (auto* Health = OtherActor->FindComponentByClass<UHealthComponent>();
		IsValid(Health))
	{
		Health->Heal(HealAmount, HealingTime);
	}

	Destroy();
}

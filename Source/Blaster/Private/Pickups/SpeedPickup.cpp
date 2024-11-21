// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SpeedPickup.h"
#include "Components/BuffComponent.h"


ASpeedPickup::ASpeedPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	WalkSpeedBoost = 1000.f;
	CrouchSpeedBoost = 600.f;
	Duration = 5.f;
}

void ASpeedPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                        AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                        bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (auto* BuffComponent = OtherActor->FindComponentByClass<UBuffComponent>();
		IsValid(BuffComponent))
	{
		BuffComponent->BoostSpeed(WalkSpeedBoost, CrouchSpeedBoost, Duration);
	}

	Destroy();
}

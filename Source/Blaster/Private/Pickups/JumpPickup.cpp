// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/JumpPickup.h"
#include "Components/BuffComponent.h"

AJumpPickup::AJumpPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	JumpZVelocityBoost = 4000.f;
	Duration = 5.f;
}

void AJumpPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                       AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                       bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (auto* BuffComponent = OtherActor->FindComponentByClass<UBuffComponent>();
		IsValid(BuffComponent))
	{
		BuffComponent->BoostJump(JumpZVelocityBoost, Duration);
	}

	Destroy();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Pickups/AmmoPickup.h"

#include "Components/CombatComponent.h"


AAmmoPickup::AAmmoPickup()
{
	AmmoAmount = 0;
	WeaponType = EWeaponType::EWT_AssaultRifle;
}

void AAmmoPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                       AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                       int32 OtherBodyIndex, bool bFromSweep,
                                       const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent,
	                            OtherActor, OtherComp,
	                            OtherBodyIndex, bFromSweep, SweepResult);

	if (auto* CombatComponent = OtherActor->FindComponentByClass<UCombatComponent>();
		IsValid(CombatComponent))
	{
		CombatComponent->PickupAmmo(WeaponType, AmmoAmount);
	}
	Destroy();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Character = nullptr;
	EquippedWeapon = nullptr;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABlasterCharacter>(GetOwner());
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (not IsValid(Character) or not IsValid(WeaponToEquip)) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetState(EWeaponState::EWS_Equipped);

	auto* Socket = Character->GetMesh()->GetSocketByName(EquipSocketName);
	Socket->AttachActor(EquippedWeapon, Character->GetMesh());

	EquippedWeapon->SetOwner(Character);
	// EquippedWeapon->ShowPickupWidget(false);
}

bool UCombatComponent::IsWeaponEquipped() const
{
	return IsValid(EquippedWeapon);
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Character = nullptr;
	EquippedWeapon = nullptr;
	bAiming = false;
	bFiring = false;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABlasterCharacter>(GetOwner());

	SetMaxWalkSpeed(BaseWalkSpeed);
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
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;

	ServerEquipWeapon(WeaponToEquip);
}

void UCombatComponent::ServerEquipWeapon_Implementation(AWeapon* WeaponToEquip)
{
	NetMulticastEquipWeapon(WeaponToEquip);
}

void UCombatComponent::NetMulticastEquipWeapon_Implementation(AWeapon* WeaponToEquip)
{
	EquippedWeapon = WeaponToEquip;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	ServerSetAiming(bIsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	NetMulticastSetAiming(bIsAiming);
}

void UCombatComponent::NetMulticastSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	
	const float& NewWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed; 
	SetMaxWalkSpeed(NewWalkSpeed);
}

void UCombatComponent::SetFiring(bool bIsFiring)
{
	bFiring = bIsFiring;

	if (bFiring)
	{
		ServerFire();	
	}
}

void UCombatComponent::ServerFire_Implementation()
{
	NetMulticastFire();
}

void UCombatComponent::NetMulticastFire_Implementation()
{
	if (IsValid(Character))
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire();
	}
}

void UCombatComponent::SetMaxWalkSpeed(const float& Speed)
{
	if (IsValid(Character))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = Speed;
	}
}

bool UCombatComponent::IsWeaponEquipped() const
{
	return IsValid(EquippedWeapon);
}

bool UCombatComponent::IsAiming() const
{
	return bAiming;
}

bool UCombatComponent::IsFiring() const
{
	return bFiring;
}

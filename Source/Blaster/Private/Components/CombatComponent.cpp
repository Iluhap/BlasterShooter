// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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
	HitTraceLength = 80.f * 1000.f;
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

		FHitResult TraceResult;
		TraceUnderCrosshair(TraceResult);

		ServerFire(TraceResult.ImpactPoint);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	NetMulticastFire(HitTarget);
}

void UCombatComponent::NetMulticastFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	if (IsValid(Character) and IsValid(EquippedWeapon))
	{
		Character->PlayFireMontage(bAiming);
		
		EquippedWeapon->Fire(HitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& HitResult) const
{
	FVector2D ViewportSize;
	if (IsValid(GEngine) and IsValid(GEngine->GameViewport))
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	const FVector2D CrossHairLocation { ViewportSize.X / 2, ViewportSize.Y / 2 };

	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;

	const auto* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (UGameplayStatics::DeprojectScreenToWorld(
			PlayerController,
			CrossHairLocation,
			CrossHairWorldPosition, CrossHairWorldDirection)
	)
	{
		const FVector Start = CrossHairWorldPosition;
		const FVector End = Start + CrossHairWorldDirection * HitTraceLength;

		const bool HasHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start, End,
			ECC_Visibility);

		if (not HasHit)
		{
			HitResult.ImpactPoint = End;
		}
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

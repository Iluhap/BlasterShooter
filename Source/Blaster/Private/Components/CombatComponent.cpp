// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Character = nullptr;
	Controller = nullptr;
	EquippedWeapon = nullptr;
	bAiming = false;
	bFiring = false;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
	HitTraceLength = 80.f * 1000.f;

	CrosshairVelocityFactorMax = 1.0f;
	CrosshairInAirFactorMax = 2.25f;
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

	if (Character)
		Controller = Cast<ABlasterPlayerController>(Character->GetController());

	SetMaxWalkSpeed(BaseWalkSpeed);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCrosshairFactors(DeltaTime);
	SetHUDCrosshair(DeltaTime);
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

void UCombatComponent::SetHUDCrosshair(float DeltaTime)
{
	if (IsValid(Controller))
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (IsValid(HUD))
		{
			FHUDPackage HUDPackage {};
			if (IsValid(EquippedWeapon))
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
			}
			
			HUDPackage.CrosshairSpread = CrosshairVelocityFactor + CrosshairInAirFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::UpdateCrosshairFactors(float DeltaTime)
{
	UpdateCrosshairVelocityFactor();
	UpdateCrosshairInAirFactor(DeltaTime);
}

void UCombatComponent::UpdateCrosshairVelocityFactor()
{
	const FVector2D WalkSpeedRange { 0.f, Character->GetCharacterMovement()->MaxWalkSpeed };
	const FVector2D VelocityMultiplierRange { 0.f, 1.f };

	FVector Velocity = Character->GetVelocity();
	Velocity.Z = 0.f;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange, VelocityMultiplierRange, Velocity.Size()
	);
}

void UCombatComponent::UpdateCrosshairInAirFactor(float DeltaTime)
{
	if (Character->GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, CrosshairInAirFactorMax, DeltaTime, 2.25f);
	}
	else
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
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

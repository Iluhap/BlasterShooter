// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "Interfaces/InteractWithCrosshairInterface.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Character = nullptr;
	Controller = nullptr;
	HUD = nullptr;
	EquippedWeapon = nullptr;
	bAiming = false;
	bFiring = false;

	DefaultFOV = 0.f;
	CurrentFOV = 0.f;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
	HitTraceLength = 80.f * 1000.f;

	CrosshairVelocityFactor = 0.f;
	CrosshairInAirFactor = 0.f;
	CrosshairAimFactor = 0.f;
	CrosshairShootingFactor = 0.f;

	BaseCrosshairFactor = 0.5f;

	CrosshairVelocityFactorMax = 1.0f;
	CrosshairInAirFactorMax = 2.25f;
	CrosshairAimFactorMax = -0.5f;

	CrosshairShootingFactorStep = 0.3f;
	CrosshairShootingFactorMax = 3.0f;

	TraceStartOffset = 50.f;

	HUDPackage = {};

	bCanFire = true;
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
	{
		Controller = Cast<ABlasterPlayerController>(Character->GetController());
		if (const auto* FollowCamera = Character->FindComponentByClass<UCameraComponent>();
			IsValid(FollowCamera))
		{
			DefaultFOV = FollowCamera->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}

	SetMaxWalkSpeed(BaseWalkSpeed);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Character) and Character->IsLocallyControlled())
	{
		FHitResult TraceResult;
		TraceUnderCrosshair(TraceResult);

		HitTargetLocation = TraceResult.ImpactPoint;

		UpdateCrosshairFactors(DeltaTime);
		SetHUDCrosshair(DeltaTime);

		InterpFOV(DeltaTime);
	}
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
		Fire();
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

void UCombatComponent::TraceUnderCrosshair(FHitResult& HitResult)
{
	FVector2D ViewportSize;
	if (IsValid(GEngine) and IsValid(GEngine->GameViewport))
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	const FVector2D CrossHairLocation { ViewportSize.X / 2, ViewportSize.Y / 2 };

	const auto* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;

	if (UGameplayStatics::DeprojectScreenToWorld(
			PlayerController,
			CrossHairLocation,
			CrossHairWorldPosition, CrossHairWorldDirection)
	)
	{
		FVector Start = CrossHairWorldPosition;

		if (IsValid(Character))
		{
			const float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrossHairWorldDirection * (DistanceToCharacter + TraceStartOffset);
		}

		const FVector End = Start + CrossHairWorldDirection * HitTraceLength;

		if (const bool HasHit = GetWorld()->LineTraceSingleByChannel(HitResult,
		                                                             Start, End, ECC_Visibility);
			not HasHit)
		{
			HitResult.ImpactPoint = End;
		}

		UpdateCrosshairColor(HitResult);
	}
}

void UCombatComponent::SetMaxWalkSpeed(float Speed)
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
			if (IsValid(EquippedWeapon))
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
			}

			HUDPackage.CrosshairSpread = CrosshairVelocityFactor
				+ BaseCrosshairFactor
				+ CrosshairInAirFactor
				+ CrosshairAimFactor
				+ CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::UpdateCrosshairFactors(float DeltaTime)
{
	UpdateCrosshairVelocityFactor();
	UpdateCrosshairInAirFactor(DeltaTime);
	UpdateCrosshairAimFactor(DeltaTime);
	UpdateCrosshairShootingFactor(DeltaTime);
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

void UCombatComponent::UpdateCrosshairAimFactor(float DeltaTime)
{
	if (bAiming)
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, CrosshairAimFactorMax, DeltaTime, 30.f);
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}
}

void UCombatComponent::UpdateCrosshairShootingFactor(float DeltaTime)
{
	CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 10.f);
}

void UCombatComponent::UpdateCrosshairColor(const FHitResult& TraceResult)
{
	if (TraceResult.GetActor() and TraceResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
	{
		HUDPackage.CrosshairColor = FLinearColor::Red;
	}
	else
	{
		HUDPackage.CrosshairColor = FLinearColor::White;
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (not IsValid(EquippedWeapon))
		return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(),
									  DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV,
									  DeltaTime, ZoomInterpSpeed);
	}
	if (IsValid(Character))
	{
		if (auto* Camera = Character->FindComponentByClass<UCameraComponent>();
			IsValid(Camera))
		{
			Camera->SetFieldOfView(CurrentFOV);
		}
	}
}

void UCombatComponent::Fire()
{
	if (IsWeaponEquipped() and bCanFire)
	{
		bCanFire = false;

		ServerFire(HitTargetLocation);

		CrosshairShootingFactor = FMath::Min(CrosshairShootingFactor + CrosshairShootingFactorStep,
		                                     CrosshairShootingFactorMax);
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (not IsWeaponEquipped())
		return;

	const float FireDelay = 60.f / EquippedWeapon->GetFireRate();

	GetWorld()->GetTimerManager().SetTimer(FireTimerHandle,
	                                       this, &UCombatComponent::FireTimerFinished,
	                                       FireDelay, true);
}

void UCombatComponent::FireTimerFinished()
{
	if (not IsWeaponEquipped())
		return;

	bCanFire = true;
	if (bFiring and EquippedWeapon->IsAutomatic())
	{
		Fire();
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

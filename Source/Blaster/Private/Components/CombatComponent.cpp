// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameMode/BlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "Interfaces/InteractWithCrosshairInterface.h"
#include "Weapon/WeaponTypes.h"
#include "Sound/SoundCue.h"
#include "Weapon/Projectile.h"
#include "Windows/WindowsApplication.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Character = nullptr;
	Controller = nullptr;
	HUD = nullptr;
	EquippedWeapon = nullptr;
	SecondaryWeapon = nullptr;
	bAiming = false;
	bLocalAimingPressed = false;
	bFiring = false;

	RightHandSocket = "RightHandSocket";
	LeftHandSocket = "LeftHandSocket";
	SecondaryWeaponSocket = "SecondaryWeaponSocket";

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

	MaxGrenades = 4;

	ActiveCarriedAmmo = 0;
	Grenades = MaxGrenades;

	CombatState = ECombatState::ECS_Unoccupied;

	ZoomFOV = 30.f;
	ZoomInterpSpeed = 20.f;

	AssaultRifleStartAmmo = 90;
	RocketLauncherStartAmmo = 10;
	PistolStartAmmo = 30;
	SubmachineGunStartAmmo = 60;
	ShotgunStartAmmo = 20;
	SniperRifleStartAmmo = 10;
	GrenadeLauncherStartAmmo = 15;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, ActiveCarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
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

		if (Character->HasAuthority())
			InitializeCarriedAmmo();
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
	if (not IsValid(WeaponToEquip))
		return;

	if (CombatState != ECombatState::ECS_Unoccupied)
		return;

	if (IsValid(EquippedWeapon) and not IsValid(SecondaryWeapon))
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}

	if (IsValid(Character))
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::SwapWeapons()
{
	if (not ShouldSwapWeapons())
		return;

	std::swap(EquippedWeapon, SecondaryWeapon);

	EquippedWeapon->SetState(EWeaponState::EWS_Equipped);
	AttachActorToSocket(EquippedWeapon, RightHandSocket);
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->UpdateHUDAmmo();
	UpdateActiveCarriedAmmo();
	PlayEquipSound(EquippedWeapon);
	ReloadEmptyWeapon();

	SecondaryWeapon->SetState(EWeaponState::EWS_Equipped_Secondary);
	AttachActorToSocket(SecondaryWeapon, SecondaryWeaponSocket);
	SecondaryWeapon->SetOwner(Character);
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (not IsValid(WeaponToEquip))
		return;

	DropWeapon(EquippedWeapon);

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetState(EWeaponState::EWS_Equipped);

	AttachActorToSocket(EquippedWeapon, RightHandSocket);

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->UpdateHUDAmmo();

	UpdateActiveCarriedAmmo();
	PlayEquipSound(WeaponToEquip);
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (not IsValid(WeaponToEquip))
		return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetState(EWeaponState::EWS_Equipped_Secondary);

	AttachActorToSocket(SecondaryWeapon, SecondaryWeaponSocket);
	SecondaryWeapon->SetOwner(Character);
	PlayEquipSound(WeaponToEquip);
}

void UCombatComponent::SpawnDefaultWeapon()
{
	auto* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));

	if (not IsValid(BlasterGameMode)) // Default weapon should be spawned only at BlasterGameMode
		return;

	if (IsValid(DefaultWeaponClass))
	{
		auto* DefaultWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
		DefaultWeapon->SetDestroyOnDrop(true);
		EquipWeapon(DefaultWeapon);
	}
}

void UCombatComponent::DropWeapons()
{
	DropWeapon(EquippedWeapon);
	DropWeapon(SecondaryWeapon);
}

void UCombatComponent::DropWeapon(AWeapon* Weapon)
{
	if (IsValid(Weapon))
	{
		Weapon->Dropped();
	}
}

void UCombatComponent::AttachActorToSocket(AActor* Actor, const FName& AttachSocketName)
{
	if (not IsValid(Character) or not IsValid(Character->GetMesh())
		or not IsValid(Actor))
		return;

	if (auto* Socket = Character->GetMesh()->GetSocketByName(AttachSocketName);
		IsValid(Socket))
	{
		Socket->AttachActor(Actor, Character->GetMesh());
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShow)
{
	if (not IsValid(Character))
		return;

	if (auto* GrenadeMesh = Character->GetAttachedGrenade();
		IsValid(GrenadeMesh))
	{
		GrenadeMesh->SetVisibility(bShow);
	}
}

void UCombatComponent::Reload()
{
	if (ActiveCarriedAmmo > 0
		and IsValid(EquippedWeapon) and not EquippedWeapon->IsFull()
		and CombatState == ECombatState::ECS_Unoccupied
		and not bLocalReloading)
	{
		HandleReload();
		ServerReload();

		bLocalReloading = true;
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (not IsValid(Character))
		return;

	CombatState = ECombatState::ECS_Reloading;
	if (not Character->IsLocallyControlled())
	{
		HandleReload();
	}
}

void UCombatComponent::FinishReloading()
{
	if (not IsValid(Character))
		return;

	bLocalReloading = false;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFiring)
	{
		Fire();
	}
}

void UCombatComponent::HandleReload()
{
	if (IsValid(Character))
	{
		Character->PlayReloadMontage();
	}
}

int32 UCombatComponent::AmountToReload() const
{
	if (not IsValid(EquippedWeapon))
		return 0;

	const int32 RoomInMagazine = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

	if (const auto* CarriedAmmoPtr = CarriedAmmoMap.Find(EquippedWeapon->GetWeaponType());
		CarriedAmmoPtr)
	{
		const int32 AmountCarried = *CarriedAmmoPtr;
		const int32 Least = FMath::Min(RoomInMagazine, AmountCarried);

		return FMath::Clamp(RoomInMagazine, 0, Least);
	}

	return 0;
}

void UCombatComponent::UpdateAmmoValues()
{
	if (not IsValid(Character)
		or not IsValid(EquippedWeapon))
		return;

	const int32 ReloadAmount = AmountToReload();

	if (auto* CarriedAmmoPtr = CarriedAmmoMap.Find(EquippedWeapon->GetWeaponType());
		CarriedAmmoPtr)
	{
		*CarriedAmmoPtr -= ReloadAmount;
		ActiveCarriedAmmo = *CarriedAmmoPtr;
	}

	UpdateHUDActiveCarriedAmmo();

	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (not IsValid(Character)
		or not IsValid(EquippedWeapon))
		return;

	if (auto* CarriedAmmoPtr = CarriedAmmoMap.Find(EquippedWeapon->GetWeaponType());
		CarriedAmmoPtr)
	{
		*CarriedAmmoPtr -= 1;
		ActiveCarriedAmmo = *CarriedAmmoPtr;
	}

	UpdateHUDActiveCarriedAmmo();

	EquippedWeapon->AddAmmo(1);
	bCanFire = true;

	if (EquippedWeapon->IsFull() or ActiveCarriedAmmo == 0)
	{
		// Jump to ShotgunEnd section
		JumpToShotgunEnd();
	}
}

void UCombatComponent::PlayEquipSound(const AWeapon* Weapon) const
{
	if (IsValid(Character)
		and IsValid(Weapon) and IsValid(Weapon->EquipSound))
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Weapon->EquipSound,
			Character->GetActorLocation());
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	SetAimingImpl(bIsAiming);
	ServerSetAiming(bIsAiming);

	if (IsValid(Character) and IsValid(EquippedWeapon))
	{
		if (Character->IsLocallyControlled())
		{
			bLocalAimingPressed = bIsAiming;

			if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
			{
				Character->ShowSniperScopeWidget(bIsAiming);
			}
		}
	}
}

void UCombatComponent::OnRep_Aiming()
{
	if (IsValid(Character) and Character->IsLocallyControlled())
	{
		bAiming = bLocalAimingPressed;
	}
}

void UCombatComponent::SetAimingImpl(bool bIsAiming)
{
	bAiming = bIsAiming;
	const float& NewWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	SetMaxWalkSpeed(NewWalkSpeed);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	SetAimingImpl(bIsAiming);
}

void UCombatComponent::Fire()
{
	if (IsWeaponEquipped() and CanFire())
	{
		bCanFire = false;

		LocalFire(HitTargetLocation);

		CrosshairShootingFactor = FMath::Min(CrosshairShootingFactor + CrosshairShootingFactorStep,
		                                     CrosshairShootingFactorMax);
		StartFireTimer();
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& HitTarget)
{
	if (not IsValid(EquippedWeapon))
		return;

	const bool bUnoccupied = CombatState == ECombatState::ECS_Unoccupied;
	const bool bReloadingShotgun =
		CombatState == ECombatState::ECS_Reloading and EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun;

	if (IsValid(Character) and (bUnoccupied or bReloadingShotgun))
	{
		Character->PlayFireMontage(bAiming);
		ServerPlayFireMontage();
		EquippedWeapon->Fire(HitTarget);

		if (bReloadingShotgun)
		{
			CombatState = ECombatState::ECS_Unoccupied;
		}
	}
}

void UCombatComponent::ServerPlayFireMontage_Implementation()
{
	NetMulticastPlayFireMontage();
}

void UCombatComponent::NetMulticastPlayFireMontage_Implementation()
{
	if (IsValid(Character) and Character->IsLocallyControlled())
		return;

	Character->PlayFireMontage(bAiming);
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
	if (IsFiring() and EquippedWeapon->IsAutomatic())
	{
		Fire();
	}
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire() const
{
	if (not IsValid(EquippedWeapon))
		return false;

	if (bLocalReloading)
		return false;

	if (not EquippedWeapon->IsEmpty()
		and bCanFire
		and CombatState == ECombatState::ECS_Reloading
		and EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		return true;

	return not EquippedWeapon->IsEmpty()
		and bCanFire
		and CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::SetFiring(bool bIsFiring)
{
	bFiring = bIsFiring;

	if (bFiring)
	{
		Fire();
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

void UCombatComponent::ShotgunShellReload()
{
	if (IsValid(Character) and Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	if (auto* AnimInstance = Character->GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(Character->GetReloadMontage()))
	{
		const FName SectionName { "ShotgunEnd" };

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0)
		return;

	if (CombatState != ECombatState::ECS_Unoccupied)
		return;

	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (not IsValid(Character))
		return;

	Character->PlayThrowGrenadeMontage();
	AttachActorToSocket(EquippedWeapon, LeftHandSocket);
	ShowAttachedGrenade(true);

	if (not Character->HasAuthority())
	{
		ServerThrowGrenade();
	}
	else
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0)
		return;

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToSocket(EquippedWeapon, LeftHandSocket);

		ShowAttachedGrenade(true);
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToSocket(EquippedWeapon, RightHandSocket);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	if (IsValid(Character) and Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTargetLocation);
	}
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] += AmmoAmount;
	}
	UpdateActiveCarriedAmmo();

	if (IsValid(EquippedWeapon)
		and EquippedWeapon->IsEmpty()
		and EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (not IsValid(Character))
		return;

	if (const auto* Grenade = Character->GetAttachedGrenade();
		IsValid(Grenade))
	{
		const FVector SpawnLocation = Grenade->GetComponentLocation();
		const FVector ToTarget = Target - SpawnLocation;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;

		GetWorld()->SpawnActor<AProjectile>(
			GrenadeClass,
			SpawnLocation, ToTarget.Rotation(),
			SpawnParams);
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

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, AssaultRifleStartAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, RocketLauncherStartAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, PistolStartAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, SubmachineGunStartAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, ShotgunStartAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, SniperRifleStartAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, GrenadeLauncherStartAmmo);
}

void UCombatComponent::UpdateActiveCarriedAmmo()
{
	if (not IsValid(EquippedWeapon))
		return;

	const auto WeaponType = EquippedWeapon->GetWeaponType();
	if (const auto* AmmoAmount = CarriedAmmoMap.Find(WeaponType))
	{
		ActiveCarriedAmmo = *AmmoAmount;

		UpdateHUDActiveCarriedAmmo();
	}
}

void UCombatComponent::UpdateHUDActiveCarriedAmmo()
{
	Character = Cast<ABlasterCharacter>(GetOwner());
	if (not IsValid(Character))
		return;

	Controller = Cast<ABlasterPlayerController>(Character->GetController());

	if (not IsValid(Controller))
		return;

	Controller->SetHUDCarriedAmmo(ActiveCarriedAmmo);
}

void UCombatComponent::UpdateHUDGrenades()
{
	if (not IsValid(Character))
		return;

	Controller = Cast<ABlasterPlayerController>(Character->GetController());

	if (not IsValid(Controller))
		return;

	Controller->SetHUDGrenades(Grenades);
}

void UCombatComponent::OnRep_ActiveCarriedAmmo()
{
	UpdateHUDActiveCarriedAmmo();

	const bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading
		and IsValid(EquippedWeapon)
		and EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun
		and ActiveCarriedAmmo == 0;

	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		{
			if (IsValid(Character) and not Character->IsLocallyControlled())
			{
				HandleReload();
			}
			break;
		}
	case ECombatState::ECS_Unoccupied:
		{
			if (IsFiring())
			{
				Fire();
			}
			break;
		}
	case ECombatState::ECS_ThrowingGrenade:
		{
			if (IsValid(Character) and not Character->IsLocallyControlled())
			{
				Character->PlayThrowGrenadeMontage();
				AttachActorToSocket(EquippedWeapon, LeftHandSocket);
				ShowAttachedGrenade(true);
			}
			break;
		}
	default:
		{
			break;
		}
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (IsValid(EquippedWeapon))
	{
		EquippedWeapon->SetState(EWeaponState::EWS_Equipped);
		AttachActorToSocket(EquippedWeapon, RightHandSocket);
		PlayEquipSound(EquippedWeapon);
		EquippedWeapon->EnableCustomDepth(false);
		EquippedWeapon->UpdateHUDAmmo();

		if (IsValid(Character))
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->bUseControllerRotationYaw = false;
		}
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (IsValid(SecondaryWeapon))
	{
		SecondaryWeapon->SetState(EWeaponState::EWS_Equipped_Secondary);

		AttachActorToSocket(SecondaryWeapon, SecondaryWeaponSocket);

		PlayEquipSound(SecondaryWeapon);
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

bool UCombatComponent::ShouldSwapWeapons() const
{
	return IsValid(EquippedWeapon) and IsValid(SecondaryWeapon)
		and CombatState == ECombatState::ECS_Unoccupied;
}

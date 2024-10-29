#include "Character/BlasterCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Blaster/Blaster.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CameraArm = CreateDefaultSubobject<USpringArmComponent>("Camera Arm");
	CameraArm->SetupAttachment(GetMesh());
	CameraArm->TargetArmLength = 300.f;
	CameraArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("Follow Camera");
	FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Overhead Widget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>("Combat Component");

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ABlasterCharacter::Jump);
	Input->BindAction(EquipAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Equip);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABlasterCharacter::OnCrouch);

	Input->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::StartAim);
	Input->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::StopAim);

	Input->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::StartFire);
	Input->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::StopFire);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Combat);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() > ROLE_SimulatedProxy and IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementRep += DeltaTime;
		if (TimeSinceLastMovementRep > 0.25f)
			OnRep_ReplicateMovement();

		CalculateAimOffsetPitch();
	}

	HideCharacterIfCameraClose();
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
}

void ABlasterCharacter::PlayFireMontage(bool IsAiming)
{
	if (not IsValid(Combat) or not Combat->IsWeaponEquipped())
		return;

	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(FireWeaponMontage))
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = IsAiming ? FName { "RifleAim" } : FName { "RifleHip" };

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (not IsValid(Combat) or not Combat->IsWeaponEquipped())
		return;

	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(HitReactMontage))
	{
		AnimInstance->Montage_Play(HitReactMontage);
		const FName SectionName = FName { "FromFront" };

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::CalculateAimOffsetPitch()
{
	AimOffsetPitch = GetBaseAimRotation().Pitch;

	if (AimOffsetPitch > 90.f and not IsLocallyControlled())
	{
		const FVector2D InRange { 270.f, 360.f };
		const FVector2D OutRange { -90.f, 0.f };

		AimOffsetPitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AimOffsetPitch);
	}
}

float ABlasterCharacter::GetSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// For playing server only
	if (IsValid(OverlappingWeapon))
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	// For playing server only
	if (IsLocallyControlled())
	{
		if (IsValid(OverlappingWeapon))
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_ReplicateMovement()
{
	Super::OnRep_ReplicateMovement();

	SimProxiesTurn();

	TimeSinceLastMovementRep = 0;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (IsValid(OverlappingWeapon))
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (IsValid(LastWeapon))
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D AxisValue = Value.Get<FVector2D>();

	if (IsValid(Controller))
	{
		const FRotator Yaw = { 0.f, Controller->GetControlRotation().Yaw, 0.f };
		const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, AxisValue.Y);
		AddMovementInput(Right, AxisValue.X);
	}
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D AxisValue = Value.Get<FVector2D>();
	AddControllerYawInput(AxisValue.X);
	AddControllerPitchInput(AxisValue.Y);
}

void ABlasterCharacter::Equip()
{
	if (IsValid(Combat))
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquip();
			Combat->EquipWeapon(OverlappingWeapon);
		}
	}
}

void ABlasterCharacter::ServerEquip_Implementation()
{
	if (IsValid(Combat))
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::OnCrouch()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::StartAim()
{
	if (IsValid(Combat))
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::StopAim()
{
	if (IsValid(Combat))
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::StartFire()
{
	if (IsValid(Combat))
	{
		Combat->SetFiring(true);
	}
}

void ABlasterCharacter::StopFire()
{
	if (IsValid(Combat))
	{
		Combat->SetFiring(false);
	}
}

void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat and Combat->IsWeaponEquipped();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat and not Combat->IsWeaponEquipped())
		return;

	const float Speed = GetSpeed();
	const bool IsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f and not IsInAir)
	{
		bRotateRootBone = true;

		const FRotator CurrentAimRotation { 0.f, GetBaseAimRotation().Yaw, 0.f };
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(
			CurrentAimRotation, StartingAimRotation);

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAimOffsetYaw = AimOffsetYaw;
		}

		AimOffsetYaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f or IsInAir)
	{
		bRotateRootBone = false;

		StartingAimRotation = { 0.f, GetBaseAimRotation().Yaw, 0.f };
		AimOffsetYaw = 0.f;
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAimOffsetPitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (not IsValid(Combat) or not Combat->IsWeaponEquipped())
		return;

	bRotateRootBone = false;

	if (GetSpeed() > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	CalculateAimOffsetPitch();

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();

	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AimOffsetYaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AimOffsetYaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAimOffsetYaw = FMath::FInterpTo(InterpAimOffsetYaw, 0.f, DeltaTime, 4.f);
		AimOffsetYaw = InterpAimOffsetYaw;

		if (FMath::Abs(AimOffsetYaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;

			StartingAimRotation = { 0.f, GetBaseAimRotation().Yaw, 0.f };
		}
	}
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (FVector::Distance(FollowCamera->GetComponentLocation(), GetActorLocation()) < CameraThreshold)
	{
		HideCharacter(true);
	}
	else
	{
		HideCharacter(false);
	}
}

void ABlasterCharacter::HideCharacter(bool bHide)
{
	if (not IsLocallyControlled())
		return;

	GetMesh()->SetVisibility(not bHide);

	if (IsValid(Combat) and Combat->IsWeaponEquipped())
	{
		if (auto* WeaponMesh = Combat->GetEquippedWeapon()->FindComponentByClass<USkeletalMeshComponent>();
			IsValid(WeaponMesh))
		{
			WeaponMesh->bOwnerNoSee = bHide;
		}
	}
}

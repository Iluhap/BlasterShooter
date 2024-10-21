#include "Character/BlasterCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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
	Combat->SetAiming(true);
}

void ABlasterCharacter::StopAim()
{
	Combat->SetAiming(false);
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat and Combat->IsWeaponEquipped();
}

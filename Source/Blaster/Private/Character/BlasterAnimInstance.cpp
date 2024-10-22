// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"

#include "Character/BlasterCharacter.h"
#include "Components/CombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"


void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UBlasterAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (not IsValid(BlasterCharacter))
	{
		return;
	}

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;

	Speed = Velocity.Size();
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

	const float CurrentAcceleration = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size();
	bIsAccelerating = CurrentAcceleration > 0.f;

	if (const auto* CombatComponent = BlasterCharacter->FindComponentByClass<UCombatComponent>();
		IsValid(CombatComponent))
	{
		bWeaponEquipped = CombatComponent->IsWeaponEquipped();
		EquippedWeapon = CombatComponent->GetEquippedWeapon();
		bIsAiming = CombatComponent->IsAiming();
	}
	bIsCrouched = BlasterCharacter->bIsCrouched;

	// Offset Yaw for Strafing
	const FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());

	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 0.5f);

	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AimOffsetYaw = BlasterCharacter->GetAimOffsetYaw();
	AimOffsetPitch = BlasterCharacter->GetAimOffsetPitch();

	SetLeftHandTransform();

	TurningInPlace = BlasterCharacter->GetTurningInPlace();
}

void UBlasterAnimInstance::SetLeftHandTransform()
{
	if (IsValid(BlasterCharacter) and IsValid(EquippedWeapon))
	{
		if (const auto* WeaponMesh = EquippedWeapon->FindComponentByClass<USkeletalMeshComponent>();
			IsValid(WeaponMesh))
		{
			LeftHandTransform = WeaponMesh->GetSocketTransform(FName { "LeftHandSocket" }, RTS_World);

			if (const auto* CharacterMesh = BlasterCharacter->FindComponentByClass<USkeletalMeshComponent>();
				IsValid(CharacterMesh))
			{
				FVector OutPosition;
				FRotator OutRotator;
				CharacterMesh->TransformToBoneSpace(FName { "hand_r" },
				                                    LeftHandTransform.GetLocation(), FRotator::ZeroRotator,
				                                    OutPosition, OutRotator);

				LeftHandTransform.SetLocation(OutPosition);
				LeftHandTransform.SetRotation(FQuat { OutRotator });
			}
		}
	}
}

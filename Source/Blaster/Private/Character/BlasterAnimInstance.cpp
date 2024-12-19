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

	UpdateMovementVariables();
	UpdateCombatComponentVariables();
	UpdateAimOffset(DeltaSeconds);

	UpdateLeftHandTransform();
	UpdateRightHandTransform(DeltaSeconds);
}

void UBlasterAnimInstance::UpdateLeftHandTransform()
{
	if (IsValid(BlasterCharacter) and IsValid(EquippedWeapon))
	{
		const auto* WeaponMesh = EquippedWeapon->FindComponentByClass<USkeletalMeshComponent>();
		const auto* CharacterMesh = BlasterCharacter->FindComponentByClass<USkeletalMeshComponent>();

		if (IsValid(WeaponMesh) and IsValid(CharacterMesh))
		{
			LeftHandTransform = WeaponMesh->GetSocketTransform(FName { "LeftHandSocket" }, RTS_World);

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

void UBlasterAnimInstance::UpdateRightHandTransform(float DeltaSeconds)
{
	if (not(IsValid(BlasterCharacter) and BlasterCharacter->IsLocallyControlled() and IsValid(EquippedWeapon)))
		return;

	bLocallyControlled = true;

	const auto* WeaponMesh = EquippedWeapon->FindComponentByClass<USkeletalMeshComponent>();

	if (const auto* CombatComponent = BlasterCharacter->FindComponentByClass<UCombatComponent>();
		IsValid(CombatComponent))
	{
		const FTransform RightHandTransform = WeaponMesh->GetSocketTransform(FName { "Hand_R" }, RTS_World);
		const FVector RightHandLocation = RightHandTransform.GetLocation();
		const FVector Target = RightHandLocation + (RightHandLocation - CombatComponent->GetHitTarget());

		const FRotator LookAtRotation =
			UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), Target);

		// RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation,
		//                                      DeltaSeconds, 200.f);
		RightHandRotation = LookAtRotation;
	}
}

void UBlasterAnimInstance::UpdateAimOffset(float DeltaSeconds)
{
	if (not IsValid(BlasterCharacter))
		return;

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

	TurningInPlace = BlasterCharacter->GetTurningInPlace();

	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
}

void UBlasterAnimInstance::UpdateCombatComponentVariables()
{
	if (not IsValid(BlasterCharacter))
		return;

	if (const auto* CombatComponent = BlasterCharacter->FindComponentByClass<UCombatComponent>();
		IsValid(CombatComponent))
	{
		const auto CombatState = CombatComponent->GetCombatState();

		bWeaponEquipped = CombatComponent->IsWeaponEquipped();
		EquippedWeapon = CombatComponent->GetEquippedWeapon();
		bIsAiming = CombatComponent->IsAiming();

		bUseFABRIK = CombatState == ECombatState::ECS_Unoccupied;

		if (BlasterCharacter->IsLocallyControlled()
			and CombatState != ECombatState::ECS_ThrowingGrenade
			and CombatState != ECombatState::ECS_SwapingWeapon)
		{
			bUseFABRIK = not CombatComponent->IsLocallyReloading();
		}

		bUseAimOffsets = not BlasterCharacter->IsGameplayDisabled()
			and CombatState == ECombatState::ECS_Unoccupied;
		bTransformRightHand = not BlasterCharacter->IsGameplayDisabled()
			and CombatState == ECombatState::ECS_Unoccupied;
	}
}

void UBlasterAnimInstance::UpdateMovementVariables()
{
	if (not IsValid(BlasterCharacter))
		return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;

	bIsCrouched = BlasterCharacter->bIsCrouched;
	Speed = Velocity.Size();

	const auto* CharacterMovement = BlasterCharacter->GetCharacterMovement();

	bIsInAir = CharacterMovement->IsFalling();

	const float CurrentAcceleration = CharacterMovement->GetCurrentAcceleration().Size();
	bIsAccelerating = CurrentAcceleration > 0.f;

	bEliminated = BlasterCharacter->IsEliminated();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"

#include "Character/BlasterCharacter.h"
#include "Components/CombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


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
		bIsAiming = CombatComponent->IsAiming();
	}
	bIsCrouched = BlasterCharacter->bIsCrouched;

	const FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);

	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds,1.f);

	YawOffset = DeltaRotation.Yaw;
	
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 1.f);

	Lean = FMath::Clamp(Interp, -90.f, 90.f);
}

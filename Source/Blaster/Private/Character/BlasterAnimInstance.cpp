// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"

#include "Character/BlasterCharacter.h"
#include "Components/CombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


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

	const auto* CombatComponent = BlasterCharacter->FindComponentByClass<UCombatComponent>();
	bWeaponEquipped = IsValid(CombatComponent) and CombatComponent->IsWeaponEquipped();
}



// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BuffComponent.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"


UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	Character = nullptr;

	BaseWalkSpeed = 0.f;
	BaseCrouchSpeed = 0.f;

	BaseJumpZVelocity = 0.f;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABlasterCharacter>(GetOwner());

	if (IsValid(Character))
	{
		BaseWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
		BaseCrouchSpeed = Character->GetCharacterMovement()->MaxWalkSpeedCrouched;
		BaseJumpZVelocity = Character->GetCharacterMovement()->JumpZVelocity;
	}
}


void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBuffComponent::BoostSpeed(float WalkSpeedBoost, float CrouchSpeedBoost, float Duration)
{
	MulticastSetMovementSpeed(WalkSpeedBoost, CrouchSpeedBoost);

	GetWorld()->GetTimerManager().SetTimer(SpeedBoostTimerHandle,
	                                       this, &UBuffComponent::ResetSpeed,
	                                       Duration);
}

void UBuffComponent::BoostJump(float JumpZVelocity, float Duration)
{
	MulticastJumpVelocity(JumpZVelocity);

	GetWorld()->GetTimerManager().SetTimer(JumpBoostTimerHandle,
	                                       this, &UBuffComponent::ResetJumpVelocity,
	                                       Duration);
}

void UBuffComponent::ResetSpeed()
{
	MulticastSetMovementSpeed(BaseWalkSpeed, BaseCrouchSpeed);
}

void UBuffComponent::ResetJumpVelocity()
{
	MulticastJumpVelocity(BaseJumpZVelocity);
}

void UBuffComponent::MulticastJumpVelocity_Implementation(float JumpZVelocity)
{
	if (IsValid(Character))
	{
		Character->GetCharacterMovement()->JumpZVelocity = JumpZVelocity;
	}
}

void UBuffComponent::MulticastSetMovementSpeed_Implementation(float WalkSpeed, float CrouchSpeed)
{
	if (IsValid(Character))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

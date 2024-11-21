// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BuffComponent.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"


UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	Character = nullptr;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABlasterCharacter>(GetOwner());

	if (IsValid(Character))
	{
		BaseWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
		BaseCrouchSpeed = Character->GetCharacterMovement()->MaxWalkSpeedCrouched;
	}
}


void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBuffComponent::SpeedUp(float WalkSpeedBoost, float CrouchSpeedBoost, float Duration)
{
	MulticastSetMovementSpeed(WalkSpeedBoost, CrouchSpeedBoost);

	GetWorld()->GetTimerManager().SetTimer(SpeedBoostTimerHandle,
	                                       this, &UBuffComponent::ResetSpeed,
	                                       Duration);
}

void UBuffComponent::ResetSpeed()
{
	MulticastSetMovementSpeed(BaseWalkSpeed, BaseCrouchSpeed);
}

void UBuffComponent::MulticastSetMovementSpeed_Implementation(float WalkSpeed, float CrouchSpeed)
{
	if (IsValid(Character))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

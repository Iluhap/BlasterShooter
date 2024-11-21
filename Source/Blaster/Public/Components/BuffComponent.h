// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();

public:
	void BoostSpeed(float WalkSpeedBoost, float CrouchSpeedBoost, float Duration);
	void BoostJump(float JumpZVelocity, float Duration);

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetMovementSpeed(float WalkSpeed, float CrouchSpeed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpVelocity(float JumpZVelocity);

	void ResetSpeed();
	void ResetJumpVelocity();

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	FTimerHandle SpeedBoostTimerHandle;
	FTimerHandle JumpBoostTimerHandle;

	float BaseWalkSpeed;
	float BaseCrouchSpeed;

	float BaseJumpZVelocity;
};

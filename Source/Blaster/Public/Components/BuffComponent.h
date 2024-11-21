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
	void SpeedUp(float WalkSpeedBoost, float CrouchSpeedBoost, float Duration);

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ResetSpeed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetMovementSpeed(float WalkSpeed, float CrouchSpeed);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	FTimerHandle SpeedBoostTimerHandle;

	float BaseWalkSpeed;
	float BaseCrouchSpeed;
};

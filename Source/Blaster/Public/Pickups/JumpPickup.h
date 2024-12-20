﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AmmoPickup.h"
#include "JumpPickup.generated.h"

UCLASS()
class BLASTER_API AJumpPickup : public AAmmoPickup
{
	GENERATED_BODY()

public:
	AJumpPickup();

protected:
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	                                  AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                                  bool bFromSweep, const FHitResult& SweepResult) override;

private:
	void ResetJumpVelocity();
	
private:
	UPROPERTY(EditAnywhere)
	float JumpZVelocityBoost;

	UPROPERTY(EditAnywhere)
	float Duration;

	FTimerHandle JumpBoostTimerHandle;
};

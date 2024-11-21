// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/JumpPickup.h"


AJumpPickup::AJumpPickup()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AJumpPickup::BeginPlay()
{
	Super::BeginPlay();
}

void AJumpPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

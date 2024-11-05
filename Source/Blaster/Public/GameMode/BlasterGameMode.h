﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter,
	                              class ABlasterPlayerController* VictimController,
	                              class ABlasterPlayerController* AttackerController);


	void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);
};
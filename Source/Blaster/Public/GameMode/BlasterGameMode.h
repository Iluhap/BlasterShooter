﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"


class ABlasterCharacter;
class ABlasterPlayerController;

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

public:
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	                              ABlasterPlayerController* VictimController,
	                              ABlasterPlayerController* AttackerController);


	void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

private:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime;

	float CountdownTime = 0.f;

	float LevelStartingTime;
};

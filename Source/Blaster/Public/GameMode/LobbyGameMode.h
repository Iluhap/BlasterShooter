// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"


UCLASS()
class BLASTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void StartGame();
	
private:
	UPROPERTY(EditAnywhere, Category = Players, meta=(AllowPrivateAccess = "true"))
	int32 MinNumPlayersToStart = 2;
	
	UPROPERTY(EditAnywhere, Category = Countdown, meta=(AllowPrivateAccess = "true"))
	float StartCountdown = 5.f;
	
	FTimerHandle StartTimerHandle;
};

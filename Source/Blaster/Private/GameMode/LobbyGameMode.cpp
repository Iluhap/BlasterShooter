// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (GameState->PlayerArray.Num() >= MinNumPlayersToStart)
	{
		GetWorld()->GetTimerManager().SetTimer(StartTimerHandle,
		                                       this, &ALobbyGameMode::StartGame,
		                                       StartCountdown);
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (GameState->PlayerArray.Num() < MinNumPlayersToStart)
	{
		GetWorld()->GetTimerManager().ClearTimer(StartTimerHandle);
	}
}

void ALobbyGameMode::StartGame()
{
	bUseSeamlessTravel = true;
	GetWorld()->ServerTravel("/Game/Maps/BlasterMap?listen");
}

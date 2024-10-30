// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/BlasterGameMode.h"

#include "Character/BlasterCharacter.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	if (IsValid(EliminatedCharacter))
	{
		EliminatedCharacter->Eliminate();
	}
}

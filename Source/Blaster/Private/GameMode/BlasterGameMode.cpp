// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/BlasterGameMode.h"

#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/BlasterPlayerState.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	auto* AttackerPlayerState = Cast<ABlasterPlayerState>(AttackerController->PlayerState);
	auto* VictimPlayerState = Cast<ABlasterPlayerState>(VictimController->PlayerState);

	if (IsValid(AttackerPlayerState) and AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}

	if (IsValid(EliminatedCharacter))
	{
		EliminatedCharacter->Eliminate();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (IsValid(EliminatedCharacter))
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}

	if (IsValid(EliminatedController))
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

		const auto Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/BlasterGameMode.h"

#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameState/BlasterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/BlasterPlayerState.h"


namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}


ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
	WarmupTime = 5.f;
	MatchTime = 120.f;
	CooldownTime = 10.f;
	LevelStartingTime = 0.f;
	CountdownTime = 0.f;

	MatchTimeLeft = 0.f;
	WarmupTimeLeft = 0.f;
	CooldownTimeLeft = 0.f;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (auto Iter = GetWorld()->GetPlayerControllerIterator(); Iter; ++Iter)
	{
		if (auto* Controller = Cast<ABlasterPlayerController>(*Iter);
			IsValid(Controller))
		{
			Controller->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	auto* AttackerPlayerState = Cast<ABlasterPlayerState>(AttackerController->PlayerState);
	auto* VictimPlayerState = Cast<ABlasterPlayerState>(VictimController->PlayerState);

	if (IsValid(AttackerPlayerState) and AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);

		if (auto* BlasterGameState = GetGameState<ABlasterGameState>();
			IsValid(BlasterGameState))
		{
			TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;
			for (auto LeadPlayer : BlasterGameState->TopScoringPlayers)
			{
				PlayersCurrentlyInTheLead.Add(LeadPlayer);
			}

			BlasterGameState->UpdateTopScore(AttackerPlayerState);

			if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))
			{
				if (auto* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
					IsValid(Leader))
				{
					Leader->GainedTheLead();
				}
			}
			for (auto* Player : PlayersCurrentlyInTheLead)
			{
				if (!BlasterGameState->TopScoringPlayers.Contains(Player))
				{
					if (ABlasterCharacter* Loser = Cast<ABlasterCharacter>(Player->GetPawn());
						IsValid(Loser))
					{
						Loser->LostTheLead();
					}
				}
			}
		}
	}

	if (IsValid(VictimPlayerState))
	{
		VictimPlayerState->AddToDefeats(1.f);
	}

	if (IsValid(EliminatedCharacter))
	{
		EliminatedCharacter->Eliminate(false);
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

void ABlasterGameMode::PlayerLeftGame(ABlasterCharacter* LeavingCharacter)
{
	if (not IsValid(LeavingCharacter))
		return;

	if (auto* BlasterGameState = GetGameState<ABlasterGameState>();
		IsValid(BlasterGameState))
	{
		BlasterGameState->TopScoringPlayers.Remove(LeavingCharacter->GetPlayerState<ABlasterPlayerState>());
	}

	LeavingCharacter->Eliminate(true);
}

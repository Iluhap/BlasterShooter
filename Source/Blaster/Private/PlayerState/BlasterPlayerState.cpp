// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"


void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);

	OnScoreUpdate();
}

void ABlasterPlayerState::AddToDefeats(float DefeatsAmount)
{
	Defeats += DefeatsAmount;

	OnDefeatsUpdate();
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	OnScoreUpdate();
}

void ABlasterPlayerState::OnRep_Defeats()
{
	OnDefeatsUpdate();
}

void ABlasterPlayerState::OnScoreUpdate()
{
	if (auto* BlasterController = Cast<ABlasterPlayerController>(GetPawn()->GetController());
		IsValid(BlasterController))
	{
		BlasterController->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::OnDefeatsUpdate()
{
	if (auto* BlasterController = Cast<ABlasterPlayerController>(GetPawn()->GetController());
		IsValid(BlasterController))
	{
		BlasterController->SetHUDDefeats(Defeats);
	}
}

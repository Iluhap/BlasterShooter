// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterPlayerController.h"

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	Score += ScoreAmount;

	if (auto* BlasterController = Cast<ABlasterPlayerController>(GetPawn()->GetController());
		IsValid(BlasterController))
	{
		BlasterController->SetHUDScore(Score);
	}
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	if (auto* BlasterController = Cast<ABlasterPlayerController>(GetPawn()->GetController());
		IsValid(BlasterController))
	{
		BlasterController->SetHUDScore(Score);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/LeaderCrownComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/BlasterCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameState/BlasterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/BlasterPlayerState.h"


ULeaderCrownComponent::ULeaderCrownComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	IsCrownActive = false;
}

void ULeaderCrownComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const auto* Character = GetOwnerCharacter();
		IsValid(Character))
	{
		if (const auto* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			IsValid(BlasterGameState)
			and BlasterGameState->TopScoringPlayers.Contains(Character->GetPlayerState<ABlasterPlayerState>()))
		{
			MulticastSpawnCrown();
		}
	}
}

ABlasterCharacter* ULeaderCrownComponent::GetOwnerCharacter() const
{
	return Cast<ABlasterCharacter>(GetOwner());
}

TOptional<FFXSystemSpawnParameters> ULeaderCrownComponent::GetCrownSpawnParams() const
{
	if (const auto* Character = GetOwnerCharacter();
		IsValid(Character))
	{
		return FFXSystemSpawnParameters {
			.WorldContextObject = this,
			.SystemTemplate = CrownSystem,
			.Location = Character->GetActorLocation() + FVector { 0.f, 0.f, 110.f },
			.Rotation = Character->GetActorRotation(),
			.AttachToComponent = Character->GetCapsuleComponent(),
			.bAutoDestroy = true,
			.bAutoActivate = false,
		};
	}

	return {};
}

void ULeaderCrownComponent::MulticastSpawnCrown_Implementation()
{
	if (not IsValid(CrownSystem))
		return;

	if (not IsValid(CrownComponent))
	{
		if (const auto Params = GetCrownSpawnParams();
			Params.IsSet())
		{
			CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(Params.GetValue());
		}
	}

	if (IsValid(CrownComponent) and not IsCrownActive)
	{
		CrownComponent->Activate();
		IsCrownActive = true;
	}
}

void ULeaderCrownComponent::MulticastRemoveCrown_Implementation()
{
	if (IsValid(CrownComponent))
	{
		CrownComponent->DestroyComponent();
		IsCrownActive = false;
	}
}

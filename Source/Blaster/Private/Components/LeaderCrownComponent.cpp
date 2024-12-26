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

void ULeaderCrownComponent::MulticastSpawnCrown_Implementation()
{
	if (not IsValid(CrownSystem))
		return;

	if (const auto* Character = GetOwnerCharacter();
		IsValid(Character))
	{
		if (not IsValid(CrownComponent))
		{
			CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				CrownSystem,
				Character->GetCapsuleComponent(),
				FName {},
				Character->GetActorLocation() + FVector { 0.f, 0.f, 110.f },
				Character->GetActorRotation(),
				EAttachLocation::KeepWorldPosition,
				false
			);
		}
	}

	if (IsValid(CrownComponent))
	{
		CrownComponent->Activate();
	}
}

void ULeaderCrownComponent::MulticastRemoveCrown_Implementation()
{
	if (IsValid(CrownComponent))
	{
		CrownComponent->DestroyComponent();
	}
}

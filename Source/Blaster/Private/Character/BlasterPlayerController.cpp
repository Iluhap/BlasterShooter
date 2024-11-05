// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Character/BlasterCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (const auto* LocalPlayer = GetLocalPlayer())
	{
		if (auto* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	SetHUD();
}

void ABlasterPlayerController::OnPossess(APawn* PawnToPossess)
{
	Super::OnPossess(PawnToPossess);

	SetHUD();

	if (const auto* BlasterCharacter = Cast<ABlasterCharacter>(PawnToPossess);
		IsValid(BlasterCharacter))
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::SetHUD()
{
	if (not IsValid(BlasterHUD))
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
}

bool ABlasterPlayerController::IsHUDValid() const
{
	return IsValid(BlasterHUD) and IsValid(BlasterHUD->CharacterOverlay);
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	SetHUD();

	if (not IsHUDValid())
		return;

	if (IsValid(BlasterHUD->CharacterOverlay->HealthBar)
		and IsValid(BlasterHUD->CharacterOverlay->HealthText))
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		const FString HealthText = FString::Printf(TEXT("%d/%d"),
		                                           FMath::CeilToInt(Health),
		                                           FMath::CeilToInt(MaxHealth));

		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ABlasterPlayerController::SetHUDScore(float ScoreAmount)
{
	SetHUD();

	if (not IsHUDValid())
		return;

	if (IsValid(BlasterHUD->CharacterOverlay->ScoreAmount))
	{
		const auto ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(ScoreAmount));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	SetHUD();

	if (not IsHUDValid())
		return;

	if (IsValid(BlasterHUD->CharacterOverlay->DefeatsAmount))
	{
		const auto DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	SetHUD();

	if (not IsHUDValid())
		return;

	if (IsValid(BlasterHUD->CharacterOverlay->WeaponAmmoAmount))
	{
		const auto AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	SetHUD();

	if (not IsHUDValid())
		return;

	if (IsValid(BlasterHUD->CharacterOverlay->ActiveCarriedAmmoAmount))
	{
		const auto AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->ActiveCarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

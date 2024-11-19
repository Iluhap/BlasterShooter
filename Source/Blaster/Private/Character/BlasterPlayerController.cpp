// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Character/BlasterCharacter.h"
#include "Components/CombatComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "GameMode/BlasterGameMode.h"
#include "GameState/BlasterGameState.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/BlasterPlayerState.h"


ABlasterPlayerController::ABlasterPlayerController()
{
	MatchTime = 0.f;
	WarmupTime = 0.f;
	CooldownTime = 0.f;
	LevelStartingTime = 0.f;

	CountdownInt = 0;

	ClientServerDelta = 0.f;
	TimeSyncFrequency = 5.f;
	TimeSyncRunningTime = 0.f;

	SavedHealth = 0.f;
	SavedMaxHealth = 0.f;
	SavedScoreAmount = 0.f;
	SavedDefeats = 0.f;
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, CurrentMatchState);
}

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

	ServerCheckMatchState();
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

void ABlasterPlayerController::PollInit()
{
	SetHUD();

	if (IsHUDValid())
	{
		SetHUDHealth(SavedHealth, SavedMaxHealth);
		SetHUDScore(SavedScoreAmount);
		SetHUDDefeats(SavedDefeats);

		if (const auto* CombatComponent = GetPawn()->FindComponentByClass<UCombatComponent>();
			IsValid(CombatComponent))
		{
			SetHUDGrenades(CombatComponent->GetGrenades());
		}
	}
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();

	CheckTimeSync(DeltaSeconds);
}

void ABlasterPlayerController::CheckTimeSync(float DeltaSeconds)
{
	TimeSyncRunningTime += DeltaSeconds;
	if (IsLocalController() and TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}


float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())
		return GetWorld()->GetTimeSeconds();

	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	CurrentMatchState = State;

	if (CurrentMatchState == MatchState::InProgress)
	{
		HandleMatchStarting();
	}
	else if (CurrentMatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (CurrentMatchState == MatchState::InProgress)
	{
		HandleMatchStarting();
	}
	else if (CurrentMatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchStarting()
{
	SetHUD();
	if (IsValid(BlasterHUD))
	{
		if (not IsValid(BlasterHUD->CharacterOverlay))
			BlasterHUD->AddCharacterOverlay();

		if (IsValid(BlasterHUD->Announcement))
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
		PollInit();
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	SetHUD();
	if (IsValid(BlasterHUD))
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent();

		if (IsValid(BlasterHUD->Announcement)
			and IsValid(BlasterHUD->Announcement->AnnouncementText)
			and IsValid(BlasterHUD->Announcement->InfoText))
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);

			const FString AnnouncementText = "New Match Starts In:";

			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			FString InfoTextString {};

			if (auto* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
				IsValid(BlasterGameState))
			{
				if (auto& TopScoringPlayers = BlasterGameState->TopScoringPlayers;
					TopScoringPlayers.IsEmpty())
				{
					InfoTextString = "There is no winner.";
				}
				else if (TopScoringPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner:\n%s"), *TopScoringPlayers[0]->GetPlayerName());
				}
				else
				{
					InfoTextString = "Players tied to win:\n";
					for (const auto* TiedPlayer : BlasterGameState->TopScoringPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
			}

			BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
		}
	}

	if (auto* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
		IsValid(BlasterCharacter))
	{
		BlasterCharacter->DisableGameplay();
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float ServerTime = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeOfClientRequest, ServerTime);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
                                                                     float TimeServerReceivedClientRequest)
{
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;

	const float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::SetHUD()
{
	if (not IsValid(BlasterHUD))
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	if (const auto* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		IsValid(GameMode))
	{
		WarmupTime = GameMode->GetWarmupTime();
		MatchTime = GameMode->GetMatchTime();
		CooldownTime = GameMode->GetCooldownTime();
		LevelStartingTime = GameMode->GetLevelStartingTime();
		CurrentMatchState = GameMode->GetMatchState();

		ClientJoinMidgame(CurrentMatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch,
                                                                float Warmup, float Match,
                                                                float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	CurrentMatchState = StateOfMatch;

	OnMatchStateSet(CurrentMatchState);

	if (IsValid(BlasterHUD) and CurrentMatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
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
	{
		SavedHealth = Health;
		SavedMaxHealth = MaxHealth;
		return;
	}

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
	{
		SavedScoreAmount = ScoreAmount;
		return;
	}

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
	{
		SavedDefeats = Defeats;
		return;
	}

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

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	SetHUD();

	if (not IsHUDValid())
		return;

	if (IsValid(BlasterHUD->CharacterOverlay->GrenadesText))
	{
		const auto GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
}

FText ABlasterPlayerController::MakeTimeTextFromSeconds(float TimeSeconds) const
{
	if (TimeSeconds < 0.f)
	{
		return {};
	}

	const int32 Minutes = FMath::FloorToInt(TimeSeconds / 60);
	const int32 Seconds = TimeSeconds - Minutes * 60;

	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	SetHUD();

	if (not IsHUDValid())
		return;

	if (IsValid(BlasterHUD->CharacterOverlay->MatchCountdownText))
	{
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(MakeTimeTextFromSeconds(CountdownTime));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	SetHUD();

	if (not IsValid(BlasterHUD))
		return;

	if (IsValid(BlasterHUD->Announcement)
		and IsValid(BlasterHUD->Announcement->WarmupTimerText))
	{
		BlasterHUD->Announcement->WarmupTimerText->SetText(MakeTimeTextFromSeconds(CountdownTime));
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	const float WarmupTimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	const float MatchTimeLeft = MatchTime + WarmupTimeLeft;
	const float CooldownTimeLeft = CooldownTime + MatchTimeLeft;

	if (CurrentMatchState == MatchState::WaitingToStart)
	{
		SetHUDAnnouncementCountdown(WarmupTimeLeft);
	}
	else if (CurrentMatchState == MatchState::InProgress)
	{
		SetHUDMatchCountdown(MatchTimeLeft);
	}
	else if (CurrentMatchState == MatchState::Cooldown)
	{
		SetHUDAnnouncementCountdown(CooldownTimeLeft);
	}
}

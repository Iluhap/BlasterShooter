// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/ReturnToMainMenu.h"

#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"


bool UReturnToMainMenu::Initialize()
{
	if (not Super::Initialize())
	{
		return false;
	}

	return true;
}

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (auto* PlayerController = GetController();
		IsValid(PlayerController))
	{
		FInputModeGameAndUI InputModeData;
		InputModeData.SetWidgetToFocus(TakeWidget());

		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(true);
	}

	if (ReturnButton and not ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	if (auto* SessionsSubsystem = GetSessionsSubsystem();
		IsValid(SessionsSubsystem))
	{
		SessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(
			this, &UReturnToMainMenu::OnDestroySession
		);
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	RemoveFromParent();

	if (auto* PlayerController = GetController();
		IsValid(PlayerController))
	{
		PlayerController->SetInputMode(FInputModeGameOnly {});
		PlayerController->SetShowMouseCursor(false);
	}

	if (ReturnButton and ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	if (auto* SessionsSubsystem = GetSessionsSubsystem();
		IsValid(SessionsSubsystem))
	{
		SessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(
			this, &UReturnToMainMenu::OnDestroySession
		);
	}
}


void UReturnToMainMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);

	if (IsValid(MultiplayerSessionsSubsystem))
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
	if (not bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}

	if (auto* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();
		IsValid(GameMode))
	{
		GameMode->ReturnToMainMenuHost();
	}
	else
	{
		if (auto* PlayerController = GetController();
			IsValid(PlayerController))
		{
			PlayerController->ClientReturnToMainMenuWithTextReason(FText {});
		}
	}
}

APlayerController* UReturnToMainMenu::GetController()
{
	Controller = not IsValid(Controller) ? GetWorld()->GetFirstPlayerController() : Controller;
	return Controller;
}

UMultiplayerSessionsSubsystem* UReturnToMainMenu::GetSessionsSubsystem()
{
	if (not IsValid(MultiplayerSessionsSubsystem))
	{
		if (const auto* GameInstance = GetGameInstance();
			IsValid(GameInstance))
		{
			MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		}
	}

	return MultiplayerSessionsSubsystem;
}

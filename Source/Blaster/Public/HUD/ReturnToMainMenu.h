// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

public:
	void MenuSetup();
	void MenuTearDown();

private:
	UFUNCTION()
	void ReturnButtonClicked();

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

private:
	APlayerController* GetController();
	class UMultiplayerSessionsSubsystem* GetSessionsSubsystem();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> ReturnButton;

	UPROPERTY()
	TObjectPtr<class UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	UPROPERTY()
	APlayerController* Controller;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()


public:
	void SetDisplayText(const FString& TextToDisplay);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* Pawn);

protected:
	virtual void NativeDestruct() override;

private:
	FString NetRoleToString(const ENetRole& Role);
	
private:
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* DisplayText;
};

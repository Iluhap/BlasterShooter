// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* PawnToPossess) override;

private:
	void SetHUD();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class ABlasterHUD> BlasterHUD;
};

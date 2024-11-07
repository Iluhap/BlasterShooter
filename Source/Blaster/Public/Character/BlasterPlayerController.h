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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);

	virtual float GetServerTime();
	virtual void ReceivedPlayer() override; // Sync server time with Client

	void OnMatchStateSet(FName State);

public:
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* PawnToPossess) override;

	void SetHUDTime();
	void PollInit();

	void HandleMatchInProgressState();

private:
	void SetHUD();
	bool IsHUDValid() const;
	void CheckTimeSync(float DeltaSeconds);

private:
	UFUNCTION()
	void OnRep_MatchState();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class ABlasterHUD> BlasterHUD;

	float MatchTime = 120.f;
	int32 CountdownInt;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName CurrentMatchState;

	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

private:
	bool bInitSavedHUDValues;

	float SavedHealth;
	float SavedMaxHealth;
	float SavedScoreAmount;
	int32 SavedDefeats;
};

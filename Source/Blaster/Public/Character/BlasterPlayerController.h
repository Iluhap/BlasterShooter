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
	ABlasterPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDGrenades(int32 Grenades);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);

	virtual float GetServerTime();
	virtual void ReceivedPlayer() override; // Sync server time with Client

	void OnMatchStateSet(FName State);

public:
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch,
	                       float Warmup, float Match,
	                       float Cooldown, float StartingTime);

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* PawnToPossess) override;

	void SetHUDTime();
	void PollInit();

	void HandleMatchStarting();
	void HandleCooldown();

private:
	void SetHUD();
	bool IsHUDValid() const;
	void CheckTimeSync(float DeltaSeconds);

	FText MakeTimeTextFromSeconds(float TimeSeconds) const;

	void SetShieldVisibility(ESlateVisibility Visibility);

private:
	UFUNCTION()
	void OnRep_MatchState();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class ABlasterHUD> BlasterHUD;

	float MatchTime;
	float WarmupTime;
	float CooldownTime;
	float LevelStartingTime;

	int32 CountdownInt;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName CurrentMatchState;

	float ClientServerDelta;

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency;

	float TimeSyncRunningTime;

private:
	float SavedHealth;
	float SavedMaxHealth;
	float SavedShield;
	float SavedMaxShield;
	float SavedScoreAmount;
	int32 SavedDefeats;
};

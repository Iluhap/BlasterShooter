// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"


namespace MatchState
{
	extern BLASTER_API const FName Cooldown;
}


class ABlasterCharacter;
class ABlasterPlayerController;

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

public:
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	                              ABlasterPlayerController* VictimController,
	                              ABlasterPlayerController* AttackerController);

	void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

public:
	FORCEINLINE float GetWarmupTime() const { return WarmupTime; }
	FORCEINLINE float GetMatchTime() const { return MatchTime; }
	FORCEINLINE float GetLevelStartingTime() const { return LevelStartingTime; }
	FORCEINLINE float GetCooldownTime() const { return CooldownTime; }

private:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime;
	
	float CountdownTime;

	float WarmupTimeLeft;
	float MatchTimeLeft;
	float CooldownTimeLeft;

	float LevelStartingTime;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeaderCrownComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API ULeaderCrownComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeaderCrownComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnCrown();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRemoveCrown();

private:
	class ABlasterCharacter* GetOwnerCharacter() const;

	TOptional<struct FFXSystemSpawnParameters> GetCrownSpawnParams() const;

private:
	UPROPERTY(EditAnywhere, Category=Crown)
	TObjectPtr<class UNiagaraComponent> CrownComponent;

	UPROPERTY(EditAnywhere, Category=Crown)
	TObjectPtr<class UNiagaraSystem> CrownSystem;

	bool IsCrownActive;
};

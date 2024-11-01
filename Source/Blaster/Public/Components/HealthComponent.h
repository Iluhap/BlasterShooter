// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthUpdatedHandle, const float&, Health, const float&, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeathHandle, AController*, InstigatedBy);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UHealthComponent : public UActorComponent
{
private:
	GENERATED_BODY()

public:
	UHealthComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetHealth() const { return Health; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy,
	                  AActor* DamageCauser);

public:
	FHealthUpdatedHandle OnHealthUpdate;
	FDeathHandle OnDeath;

private:
	UFUNCTION()
	void OnRep_Health();

private:
	UPROPERTY(EditAnywhere, Category=Health)
	float MaxHealth;

	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category=Health)
	float Health;

	UPROPERTY()
	AController* LastDamageInstigator;
};

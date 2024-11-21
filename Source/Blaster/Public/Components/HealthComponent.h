// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlasterTypes/HealthUpdateType.h"
#include "HealthComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthUpdatedHandle, const float&, Health, const float&, MaxHealth,
                                               EHealthUpdateType, UpdateType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeathHandle, AController*, InstigatedBy);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UHealthComponent : public UActorComponent
{
private:
	GENERATED_BODY()

public:
	UHealthComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE bool IsDead() const { return Health <= 0; }

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealingTime);

private:
	UFUNCTION()
	void OnTakeDamage(AActor* DamagedActor, float Damage,
	                  const UDamageType* DamageType,
	                  AController* InstigatedBy, AActor* DamageCauser);

	void HealTick(float DeltaTime);

public:
	FHealthUpdatedHandle OnHealthUpdate;
	FDeathHandle OnDeath;

private:
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UFUNCTION()
	void OnRep_MaxHealth();

private:
	UPROPERTY(ReplicatedUsing=OnRep_MaxHealth, EditAnywhere, Category=Health)
	float MaxHealth;

	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category=Health)
	float Health;

	UPROPERTY()
	bool bHealing;

	UPROPERTY()
	float HealingRate;

	UPROPERTY()
	float AmountToHeal;

	UPROPERTY()
	AController* LastDamageInstigator;
};

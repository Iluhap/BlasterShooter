// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlasterTypes/HealthUpdateType.h"
#include "HealthComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthUpdatedHandle, const float&, Health, const float&, MaxHealth,
                                               EHealthUpdateType, UpdateType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShieldUpdatedHandle, const float&, Shield, const float&, MaxShield);

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

	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE bool IsShieldActive() const { return Shield > 0; }

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealingTime);
	void RestoreShield(float RestoreAmount);

private:
	UFUNCTION()
	void OnTakeDamage(AActor* DamagedActor, float Damage,
	                  const UDamageType* DamageType,
	                  AController* InstigatedBy, AActor* DamageCauser);

	void HealTick(float DeltaTime);
	void ShieldTick(float DeltaTime);

	void ReduceHealth(float Damage);
	void ReduceShield(float ReduceAmount);

public:
	FHealthUpdatedHandle OnHealthUpdate;
	FShieldUpdatedHandle OnShieldUpdate;
	FDeathHandle OnDeath;

private:
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UFUNCTION()
	void OnRep_MaxHealth();

	UFUNCTION()
	void OnRep_Shield();

	UFUNCTION()
	void OnRep_MaxShield();

private:
	/*
	 * Health section
	 */

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

	/*
	 * Shield section
	 */

	UPROPERTY(ReplicatedUsing=OnRep_MaxShield, EditAnywhere, Category=Shield)
	float MaxShield;

	UPROPERTY(ReplicatedUsing=OnRep_Shield, VisibleAnywhere, Category=Shield)
	float Shield;

	UPROPERTY(EditAnywhere, Category=Shield)
	float ShieldRestorationTime;

	UPROPERTY(EditAnywhere, Category=Shield)
	float ShieldReductionTime;

	UPROPERTY()
	bool bRestoringShield;
	
	UPROPERTY()
	float ShieldRestorationRate;

	UPROPERTY()
	float ShieldReductionRate;

	UPROPERTY()
	AController* LastDamageInstigator;
};

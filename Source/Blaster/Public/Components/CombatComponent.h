// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	void EquipWeapon(class AWeapon* WeaponToEquip);
	void SetAiming(bool bIsAiming);
	void SetFiring(bool bIsFiring);

	void TraceUnderCrosshair(FHitResult& HitResult) const;
	
public:
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

private:
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(AWeapon* WeaponToEquip);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastEquipWeapon(AWeapon* WeaponToEquip);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& HitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastFire(const FVector_NetQuantize& HitTarget);

private:
	void SetMaxWalkSpeed(const float& Speed);

public: // Getters
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	bool IsFiring() const;

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY(Replicated)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(Replicated)
	bool bFiring;

private:
	UPROPERTY(EditAnywhere)
	FName EquipSocketName = "RightHandSocket";

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	UPROPERTY(EditAnywhere)
	float HitTraceLength;
};

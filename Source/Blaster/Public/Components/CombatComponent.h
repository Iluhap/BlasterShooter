// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"


enum class EWeaponType : uint8;

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
	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void SetAiming(bool bIsAiming);
	void SetFiring(bool bIsFiring);

	void TraceUnderCrosshair(FHitResult& HitResult);

public: // Getters
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	bool IsFiring() const;

	FORCEINLINE FVector GetHitTarget() const { return HitTargetLocation; }
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }

private:
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& HitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastFire(const FVector_NetQuantize& HitTarget);

	UFUNCTION(Server, Reliable)
	void ServerReload();

private:
	void SetMaxWalkSpeed(float Speed);

	void SetHUDCrosshair(float DeltaTime);

	void UpdateCrosshairFactors(float DeltaTime);
	void UpdateCrosshairVelocityFactor();
	void UpdateCrosshairInAirFactor(float DeltaTime);
	void UpdateCrosshairAimFactor(float DeltaTime);
	void UpdateCrosshairShootingFactor(float DeltaTime);
	void UpdateCrosshairColor(const FHitResult& TraceResult);

	void InterpFOV(float DeltaTime);

	void Fire();
	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire() const;

	void InitializeCarriedAmmo();
	void SetActiveCarriedAmmo();

	void HandleReload();

	int32 AmountToReload() const;

	void UpdateAmmoValues();

	void PlayEquipSound() const;

private:
	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_ActiveCarriedAmmo();

	UFUNCTION()
	void OnRep_CombatState();

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY()
	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(Replicated)
	bool bFiring;

	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	float BaseCrosshairFactor;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	float CrosshairVelocityFactorMax;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	float CrosshairInAirFactorMax;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	float CrosshairAimFactorMax;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	float CrosshairShootingFactorStep;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	float CrosshairShootingFactorMax;

	UPROPERTY(EditAnywhere, Category=Trace)
	float TraceStartOffset;

	FVector HitTargetLocation;

	FHUDPackage HUDPackage;

private:
	UPROPERTY(EditAnywhere)
	FName EquipSocketName = "RightHandSocket";

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	UPROPERTY(EditAnywhere)
	float HitTraceLength;

	// Field of view while not aiming; set to the camera's base FOV in BeginPlay
	UPROPERTY(EditAnywhere)
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category=Zoom)
	float ZoomFOV;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category=Zoom)
	float ZoomInterpSpeed;

	/*
	 * Automatic Fire
	 */

	FTimerHandle FireTimerHandle;

	bool bCanFire;

	UPROPERTY(Replicated, ReplicatedUsing=OnRep_ActiveCarriedAmmo)
	int32 ActiveCarriedAmmo;

	UPROPERTY()
	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 AssaultRifleStartAmmo;

	UPROPERTY(EditAnywhere)
	int32 RocketLauncherStartAmmo;

	UPROPERTY(EditAnywhere)
	int32 PistolStartAmmo;

	UPROPERTY(EditAnywhere)
	int32 SubmachineGunStartAmmo;

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState;
};

﻿// Fill out your copyright notice in the Description page of Project Settings.

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
	void SwapWeapons();

	void SpawnDefaultWeapon();

	void DropWeapons();

	void Reload();

	void SetAiming(bool bIsAiming);
	void SetFiring(bool bIsFiring);

	void TraceUnderCrosshair(FHitResult& HitResult);

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void FinishWeaponSwap();

	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachedWeapons();
	
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	void ThrowGrenade();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

public: // Getters
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	bool IsFiring() const;

	FORCEINLINE FVector GetHitTarget() const { return HitTargetLocation; }
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	FORCEINLINE bool IsLocallyReloading() const { return bLocalReloading; }

	bool ShouldSwapWeapons() const;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

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
	void LocalFire(const FVector_NetQuantize& HitTarget);

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire() const;

	void InitializeCarriedAmmo();
	void UpdateActiveCarriedAmmo();
	void UpdateHUDActiveCarriedAmmo();
	void UpdateHUDGrenades();

	void HandleReload();

	int32 AmountToReload() const;

	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();

	void PlayEquipSound(const AWeapon* Weapon) const;
	void DropWeapon(AWeapon* Weapon);
	void AttachActorToSocket(AActor* Actor, const FName& AttachSocketName);
	void ReloadEmptyWeapon();
	void ShowAttachedGrenade(bool bVisible);

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void UpdatePrimaryWeapon();
	
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);
	void UpdateSecondaryWeapon();

	void SetAimingImpl(bool bIsAiming);

private:
	UFUNCTION(Server, Reliable)
	void ServerPlayFireMontage();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastPlayFireMontage();

private:
	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	UFUNCTION()
	void OnRep_ActiveCarriedAmmo();

	UFUNCTION()
	void OnRep_CombatState();

	UFUNCTION()
	void OnRep_Grenades();

	UFUNCTION()
	void OnRep_Aiming();

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY()
	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing=OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	UPROPERTY(ReplicatedUsing=OnRep_Aiming)
	bool bAiming;

	bool bLocalAimingPressed;

	UPROPERTY(Replicated)
	bool bFiring;

	bool bLocalReloading;

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
	FName RightHandSocket;

	UPROPERTY(EditAnywhere)
	FName LeftHandSocket;

	UPROPERTY(EditAnywhere)
	FName SecondaryWeaponSocket;

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

	UPROPERTY(ReplicatedUsing=OnRep_Grenades)
	int32 Grenades;

	UPROPERTY(EditAnywhere)
	int32 MaxGrenades;

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

	UPROPERTY(EditAnywhere)
	int32 ShotgunStartAmmo;

	UPROPERTY(EditAnywhere)
	int32 SniperRifleStartAmmo;

	UPROPERTY(EditAnywhere)
	int32 GrenadeLauncherStartAmmo;

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState;
};

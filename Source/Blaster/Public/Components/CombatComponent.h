// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
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

	void TraceUnderCrosshair(FHitResult& HitResult);

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
	void SetMaxWalkSpeed(float Speed);

	void SetHUDCrosshair(float DeltaTime);

	void UpdateCrosshairFactors(float DeltaTime);
	void UpdateCrosshairVelocityFactor();
	void UpdateCrosshairInAirFactor(float DeltaTime);
	void UpdateCrosshairAimFactor(float DeltaTime);
	void UpdateCrosshairShootingFactor(float DeltaTime);

	void InterpFOV(float DeltaTime);

	void UpdateCrosshairColor(const FHitResult& TraceResult);

public: // Getters
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	bool IsFiring() const;

	FORCEINLINE FVector GetHitTarget() const { return HitTargetLocation; }

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY()
	class ABlasterHUD* HUD;

	UPROPERTY(Replicated)
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
	float ZoomFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category=Zoom)
	float ZoomInterpSpeed = 20.f;
};

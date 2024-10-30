// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/TurningInPlace.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairInterface.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Jump() override;

	void PlayFireMontage(bool IsAiming);

public:
	void SetOverlappingWeapon(class AWeapon* Weapon);

	virtual void OnRep_ReplicateMovement() override;

public:
	bool IsWeaponEquipped() const;
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();

	FORCEINLINE float GetAimOffsetYaw() const { return AimOffsetYaw; }
	FORCEINLINE float GetAimOffsetPitch() const { return AimOffsetPitch; }

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; };
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; };

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

private:
	void Move(const struct FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Equip();
	void OnCrouch();
	void StartAim();
	void StopAim();

	void StartFire();
	void StopFire();

	UFUNCTION(Server, Reliable)
	void ServerEquip();

private:
	void TurnInPlace(float DeltaTime);

	void HideCharacterIfCameraClose();

	void HideCharacter(bool bHide);

	void PlayHitReactMontage();

	void CalculateAimOffsetPitch();

	float GetSpeed() const;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor,
					   float Damage, const class UDamageType* DamageType,
					   class AController* InstigatedBy, AActor* DamageCauser);

	void UpdateHUDHealth();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Replicated)
	TObjectPtr<class UCombatComponent> Combat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UWidgetComponent> OverheadWidget;

private:
	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> HitReactMontage;

private:

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;
	
	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	float AimOffsetYaw;
	float InterpAimOffsetYaw;
	float AimOffsetPitch;

	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	UPROPERTY(EditAnywhere, Category=Camera)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;

	float TimeSinceLastMovementRep;

	UPROPERTY(EditAnywhere, Category="PlayerStats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category="PlayerStats")
	float Health;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> EquipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction;
};

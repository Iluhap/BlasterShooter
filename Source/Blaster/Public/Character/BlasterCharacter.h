// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Components/TimelineComponent.h"
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

	void PlayFireMontage(bool IsAiming) const;
	void PlayEliminationMontage() const;

	void Eliminate();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminate();

public:
	void SetOverlappingWeapon(class AWeapon* Weapon);

	virtual void OnRep_ReplicateMovement() override;

public:
	bool IsWeaponEquipped() const;
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();

	FORCEINLINE float GetAimOffsetYaw() const { return AimOffsetYaw; }
	FORCEINLINE float GetAimOffsetPitch() const { return AimOffsetPitch; }

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bIsEliminated; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

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

	void PlayHitReactMontage() const;

	void CalculateAimOffsetPitch();

	float GetSpeed() const;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor,
	                   float Damage, const UDamageType* DamageType,
	                   AController* InstigatedBy, AActor* DamageCauser);

	void UpdateHUDHealth();

	UFUNCTION()
	void EliminationTimerFinished();

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();

	void SetDissolveParams(const float& Dissolve, const float& Glow);

	void DisableMovement();

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

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> EliminationMontage;

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
	float CameraThreshold;

	bool bRotateRootBone;
	float TurnThreshold;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;

	float TimeSinceLastMovementRep;

	UPROPERTY(EditAnywhere, Category="PlayerStats")
	float MaxHealth;

	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category="PlayerStats")
	float Health;

	UPROPERTY(EditDefaultsOnly)
	bool bIsEliminated;

	UPROPERTY(EditAnywhere)
	float EliminationDelay;

	FTimerHandle EliminationTimerHandle;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTimelineComponent> DissolveTimeline;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> DissolveCurve;

	UPROPERTY(VisibleAnywhere, Category=Elimination)
	TObjectPtr<UMaterialInstanceDynamic> DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category=Elimination)
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	UPROPERTY(EditAnywhere)
	float BaseDissolveValue;

	UPROPERTY(EditAnywhere)
	float BaseDissolveGlow;

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

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/HealthUpdateType.h"
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

private:
	void InitHitBoxes();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Jump() override;

	void PlayFireMontage(bool IsAiming) const;
	void PlayReloadMontage() const;
	void PlayEliminationMontage() const;
	void PlayThrowGrenadeMontage() const;

	void Eliminate();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminate();

	UFUNCTION()
	void DisableGameplay();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

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
	FORCEINLINE bool IsGameplayDisabled() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return GrenadeMesh; };

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

	void Reload();
	void ThrowGrenade();

	void SwapWeapons();

	UFUNCTION(Server, Reliable)
	void ServerEquip();

	UFUNCTION(Server, Reliable)
	void ServerSwapWeapons();

private:
	void TurnInPlace(float DeltaTime);

	void HideCharacterIfCameraClose();

	void HideCharacter(bool bHide);

	void PlayHitReactMontage() const;

	void CalculateAimOffsetPitch();

	float GetSpeed() const;

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor,
	                   float Damage, const UDamageType* DamageType,
	                   AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnDeath(AController* InstigatedBy);

	UFUNCTION()
	void OnHealthUpdate(const float& NewHealth, const float& NewMaxHealth, EHealthUpdateType UpdateType);

	UFUNCTION()
	void OnShieldUpdate(const float& NewShield, const float& NewMaxShield);

	void UpdateHUDHealth();
	void SetController();
	void UpdateHUDShield();

	UFUNCTION()
	void EliminationTimerFinished();

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();

	void SetDissolveParams(const float& Dissolve, const float& Glow);

	void DisableMovement();

	void PollInit();

	void RotateInPlace(float DeltaTime);

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UBuffComponent> Buff;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UWidgetComponent> OverheadWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GrenadeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class ULagCompensationComponent> LagCompensationComponent;

private:
	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> EliminationMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> ThrowGrenadeMontage;

private:
	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

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

	bool bDisableGameplay;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ThrowGrenadeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SwapWeaponsAction;

public:
	/*
	 * Hit boxes used to server-side rewind
	 */

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitBoxes;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Head;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Pelvis;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Spine_02;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Spine_03;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> UpperArm_L;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> UpperArm_R;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> LowerArm_L;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> LowerArm_R;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Hand_L;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Hand_R;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Thigh_L;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Thigh_R;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Calf_L;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Calf_R;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Foot_R;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Foot_L;
};

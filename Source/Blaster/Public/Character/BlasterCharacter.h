// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetOverlappingWeapon(class AWeapon* Weapon);

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

	UFUNCTION(Server, Reliable)
	void ServerEquip();

public:
	bool IsWeaponEquipped() const;
	void AimOffset(const float& DeltaTime);

	FORCEINLINE float GetAimOffsetYaw() const { return AimOffsetYaw; } 
	FORCEINLINE float GetAimOffsetPitch() const { return AimOffsetPitch; } 

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
	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	float AimOffsetYaw;
	float AimOffsetPitch;

	FRotator StartingAimRotation; 

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
};

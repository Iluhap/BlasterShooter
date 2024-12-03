// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"


UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Equipped_Secondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "HitScan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "Default MAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	                        AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                        bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent,
	                      AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                      int32 OtherBodyIndex);

	void ShowPickupWidget(bool bShowWidget);

	void SetState(const EWeaponState NewState);
	void SetDestroyOnDrop(bool bDestroy);

	virtual void Fire(const FVector& HitTarget);
	virtual void LocalFire(const FVector& HitTarget);
	
	void Dropped();
	void AddAmmo(int32 AmmoAmount);

	void UpdateHUDAmmo();

	void EnableCustomDepth(bool bEnable);
	void SetMeshOutlineColor(int32 DepthValue);

public:
	bool IsEmpty() const;
	bool IsFull() const;
	TOptional<FTransform> GetMuzzleTransform() const;

public:
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; };
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; };
	FORCEINLINE float GetFireRate() const { return FireRate; };
	FORCEINLINE float IsAutomatic() const { return bAutomatic; };
	FORCEINLINE EWeaponType GetWeaponType() const { return Type; };
	FORCEINLINE EFireType GetFireType() const { return FireType; };
	FORCEINLINE int32 GetAmmo() const { return Ammo; };
	FORCEINLINE int32 GetMagazineCapacity() const { return MagazineCapacity; };

protected:
	UFUNCTION(Server, Reliable)
	virtual void ServerFire(const FVector_NetQuantize& Start, const FVector_NetQuantize& HitTarget);

	UFUNCTION(NetMulticast, Unreliable)
	virtual void NetMulticastFire(const FVector_NetQuantize& HitTarget);

protected:
	void PlayFireAnimation() const;
	void EjectCasing() const;

	virtual FVector ApplyScatterTo(const FVector& TraceStart, const FVector& HitTarget) const;

	void SpendRound();

	void OnFireEffects() const;

protected:
	virtual void OnRep_Owner() override;

private:
	UFUNCTION()
	void OnRep_State();

	UFUNCTION()
	void OnRep_Ammo();

private:
	void SetMeshCollision(bool bEnable);
	void SetOwningCharacter();
	void SetOwningController();

public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> EquipSound;

protected:
	UPROPERTY(VisibleAnywhere, Category= "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category= "Weapon Properties")
	TObjectPtr<class USphereComponent> AreaSphere;

	UPROPERTY(ReplicatedUsing=OnRep_State, VisibleAnywhere, Category= "Weapon Properties")
	EWeaponState State;

	UPROPERTY(VisibleAnywhere, Category= "Weapon Properties")
	TObjectPtr<class UWidgetComponent> PickupWidget;

	UPROPERTY(EditAnywhere, Category= "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere, Category= "Weapon Properties")
	TSubclassOf<class AAmmoCasing> AmmoCasingClass;

	UPROPERTY(EditAnywhere, Category= "Weapon Properties")
	FName AmmoEjectSocketName;

	UPROPERTY(EditAnywhere, Category= "Weapon Properties")
	FName MuzzleFlashSocketName;

	/*
	* Trace with scatter
	*/

	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float DistanceToSphere;

	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float SphereRadius;

	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	bool bUseScatter;

public:
	UPROPERTY(EditAnywhere, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairCenter;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairTop;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairBottom;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairRight;

	UPROPERTY(EditAnywhere, Category=Crosshair)
	TObjectPtr<UTexture2D> CrosshairLeft;

protected:
	UPROPERTY()
	class ABlasterPlayerController* OwningBlasterPlayerController;

	UPROPERTY()
	class ABlasterCharacter* OwningBlasterCharacter;

private:
	UPROPERTY(EditAnywhere)
	EWeaponType Type;

	UPROPERTY(EditAnywhere)
	EFireType FireType;

	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category=Properties)
	float FireRate;

	UPROPERTY(EditAnywhere, Category=Properties)
	bool bAutomatic;

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Ammo)
	int32 Ammo;

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Ammo)
	int32 MagazineCapacity;

	UPROPERTY(EditAnywhere, Category=Properties)
	bool bDestroyOnDrop;
};

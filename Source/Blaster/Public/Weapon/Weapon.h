// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"


UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};


UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

protected:
	virtual void BeginPlay() override;

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

	virtual void Fire(const FVector& HitTarget);

	void Dropped();

	void UpdateHUDAmmo();

public:
	bool IsEmpty() const;

public:
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; };
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; };
	FORCEINLINE float GetFireRate() const { return FireRate; };
	FORCEINLINE float IsAutomatic() const { return bAutomatic; };

protected:
	virtual void OnRep_Owner() override;

private:
	void SetMeshCollision(bool bEnable);

	void SpendRound();

	UFUNCTION()
	void OnRep_State();

	UFUNCTION()
	void OnRep_Ammo();

private:
	void SetOwningController();

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

private:
	UPROPERTY()
	class ABlasterPlayerController* OwningBlasterPlayerController;

	UPROPERTY()
	class ABlasterCharacter* OwningBlasterCharacter;

private:
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
};

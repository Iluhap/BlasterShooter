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

private:
	UPROPERTY(VisibleAnywhere, Category= "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category= "Weapon Properties")
	TObjectPtr<class USphereComponent> AreaSphere;

	UPROPERTY(VisibleAnywhere, Category= "Weapon Properties", Replicated)
	EWeaponState State;

	UPROPERTY(VisibleAnywhere, Category= "Weapon Properties")
	TObjectPtr<class UWidgetComponent> PickupWidget;
};

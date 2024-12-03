// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	AShotgun();

public:
	virtual void Fire(const FVector& HitTarget) override;
	virtual void LocalFire(const TArray<FVector_NetQuantize>& HitTargets);

protected:
	UFUNCTION(Server, Reliable)
	virtual void ServerFirePellets(const TArray<FVector_NetQuantize>& HitTargets);

	UFUNCTION(NetMulticast, Reliable)
	virtual void NetMulticastFirePellets(const TArray<FHitResult>& HitResults);
	
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	int32 NumberOfPellets = 10;
};

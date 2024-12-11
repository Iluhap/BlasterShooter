// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlasterTypes/LagCompensationTypes.h"
#include "Character/BlasterCharacter.h"

#include "LagCompensationComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitConfirmed,
                                             ABlasterCharacter*, HitCharacter, const FRewindResult&, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShotgunHitConfirmed, const FShotgunRewindResult&, Result);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULagCompensationComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	void SaveFramePackage(FFramePackage& Package) const;
	void ShowFramePackage(const FFramePackage& Package) const;

public:
	UFUNCTION(Server, Reliable)
	void ServerConfirmHitScan(const FHitScanRewindRequest& Request);

	UFUNCTION(Server, Reliable)
	void ServerConfirmProjectileHit(const FProjectileRewindRequest& Request);

	UFUNCTION(Server, Reliable)
	void ServerConfirmShotgunHit(const FShotgunRewindRequest& Request);

private:
	FRewindResult HitScanRewind(const FHitScanRewindRequest& Request);
	FRewindResult ProjectileRewind(const FProjectileRewindRequest& Request);
	FShotgunRewindResult ShotgunRewind(const FShotgunRewindRequest& Request);

	FRewindResult ConfirmHit(const FFramePackage& Package,
	                         ABlasterCharacter* HitCharacter,
	                         const TFunction<TOptional<FHitResult>()>& TraceFunction);

private:
	void AddFrameToHistory();
	float GetHistoryLength() const;
	FFramePackage InterpBetweenFrames(const FFramePackage& Older, const FFramePackage& Younger, float HitTime) const;
	void CacheBoxPosition(ABlasterCharacter* Character, FFramePackage& OutFramePackage);
	void MoveBoxes(ABlasterCharacter* Character, const FFramePackage& TargetPackage);
	void ResetHitBoxes(ABlasterCharacter* Character, const FFramePackage& TargetPackage);
	void EnableCharacterMeshCollision(ABlasterCharacter* Character, ECollisionEnabled::Type CollisionEnabled) const;

	void SaveFramePackage();

	TOptional<FFramePackage> GetFrameToCheck(const ABlasterCharacter* HitCharacter, float HitTime) const;

	TOptional<FHitResult> HitScanTrace(const FVector_NetQuantize& TraceStart,
	                                   const FVector_NetQuantize& HitLocation) const;

	TOptional<FHitResult> ProjectileTrace(const FVector_NetQuantize& TraceStart,
	                                      const FVector_NetQuantize100& InitialVelocity) const;

public:
	FOnHitConfirmed OnHitConfirmed;
	FOnShotgunHitConfirmed OnShotgunHitConfirmed;

private:
	TDoubleLinkedList<FFramePackage> FrameHistory;

	float MaxHistoryLength;
};

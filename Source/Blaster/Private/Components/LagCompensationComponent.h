// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/Tuple.h"
#include "LagCompensationComponent.generated.h"


USTRUCT()
struct FHitBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	float Time;

	UPROPERTY()
	TMap<FName, FHitBoxInformation> HitBoxInfo;
};

USTRUCT()
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, FHitResult> TracedHitBoxes;
};

USTRUCT()
struct FServerSideRewindResults
{
	GENERATED_BODY()
	TArray<FServerSideRewindResult> Results;
};

USTRUCT()
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<class ABlasterCharacter*, FServerSideRewindResults> TracedCharacters;
};

USTRUCT()
struct FServerSideRewindRequest
{
	GENERATED_BODY()

	UPROPERTY()
	class ABlasterCharacter* HitCharacter;

	UPROPERTY()
	FVector_NetQuantize TraceStart;

	UPROPERTY()
	FVector_NetQuantize HitLocation;

	UPROPERTY()
	float HitTime;
};

USTRUCT()
struct FShotgunServerSideRewindRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FServerSideRewindRequest> Requests;
};

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

	FServerSideRewindResult ServerSideRewind(const FServerSideRewindRequest& Request);
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const FShotgunServerSideRewindRequest& Request);

	FServerSideRewindResult ConfirmHit(const FFramePackage& Package,
	                                   ABlasterCharacter* HitCharacter,
	                                   const FVector_NetQuantize& TraceStart,
	                                   const FVector_NetQuantize& HitLocation);

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

private:
	TDoubleLinkedList<FFramePackage> FrameHistory;

	float MaxHistoryLength;
};

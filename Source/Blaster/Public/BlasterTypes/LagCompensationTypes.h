#pragma once

#include "CoreMinimal.h"

#include "LagCompensationTypes.generated.h"

USTRUCT(BlueprintType)
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

USTRUCT(BlueprintType)
struct FRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, FHitResult> TracedHitBoxes;
};

USTRUCT(BlueprintType)
struct FRewindResults
{
	GENERATED_BODY()
	TArray<FRewindResult> Results;
};

USTRUCT(BlueprintType)
struct FShotgunRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<class ABlasterCharacter*, FRewindResults> TracedCharacters;
};

USTRUCT(BlueprintType)
struct FRewindRequest
{
	GENERATED_BODY()

	UPROPERTY()
	ABlasterCharacter* HitCharacter;

	UPROPERTY()
	FVector_NetQuantize TraceStart;

	UPROPERTY()
	float HitTime;
};

USTRUCT(BlueprintType)
struct FHitScanRewindRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FRewindRequest Base;

	UPROPERTY()
	FVector_NetQuantize HitLocation;
};

USTRUCT(BlueprintType)
struct FProjectileRewindRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FRewindRequest Base;

	UPROPERTY()
	FVector_NetQuantize100 InitialVelocity;
};

USTRUCT(BlueprintType)
struct FShotgunRewindRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FHitScanRewindRequest> Requests;
};

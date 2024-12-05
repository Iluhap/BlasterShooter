// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	TMap<FName, FHitBoxInformation> HitBoxInfo;
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
	void SaveFramePackage(FFramePackage& Package);
	void ShowFramePackage(const FFramePackage& Package) const;

	void ServerSideRewind(const class ABlasterCharacter* HitCharacter,
	                      const FVector_NetQuantize& TraceStart,
	                      const FVector_NetQuantize& HitTarget,
	                      float HitTime);

private:
	void AddFrameToHistory();
	float GetHistoryLength() const;

	FFramePackage InterpBetweenFrames(const FFramePackage& Older, const FFramePackage& Younger, float HitTime);

private:
	TDoubleLinkedList<FFramePackage> FrameHistory;

	float MaxHistoryLength;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/LagCompensationComponent.h"

#include "Blaster/Blaster.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"


ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MaxHistoryLength = 4.f;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage Package;
	SaveFramePackage(Package);
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(GetOwner()) and not GetOwner()->HasAuthority())
		return;

	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (FrameHistory.Num() <= 1)
	{
		AddFrameToHistory();
	}
	else
	{
		while (GetHistoryLength() > MaxHistoryLength)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
		}

		AddFrameToHistory();
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package) const
{
	if (const auto* Character = Cast<ABlasterCharacter>(GetOwner());
		IsValid(Character))
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		for (const auto& BoxPair : Character->HitBoxes)
		{
			FHitBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package) const
{
	for (const auto& BoxPair : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(),
		             BoxPair.Value.Location,
		             BoxPair.Value.BoxExtent,
		             FQuat(BoxPair.Value.Rotation),
		             FColor::Cyan,
		             false,
		             4.f);
	}
}

void ULagCompensationComponent::ServerConfirmHitScan_Implementation(const FHitScanRewindRequest& Request)
{
	if (const auto& Result = HitScanRewind(Request);
		not Result.TracedHitBoxes.IsEmpty())
	{
		OnHitConfirmed.Broadcast(Request.Base.HitCharacter, Result);
	}
}

void ULagCompensationComponent::ServerConfirmProjectileHit_Implementation(const FProjectileRewindRequest& Request)
{
	if (const auto& Result = ProjectileRewind(Request);
		not Result.TracedHitBoxes.IsEmpty())
	{
		OnHitConfirmed.Broadcast(Request.Base.HitCharacter, Result);
	}
}

void ULagCompensationComponent::ServerConfirmShotgunHit_Implementation(const FShotgunRewindRequest& Request)
{
	if (const auto& Result = ShotgunRewind(Request);
		not Result.TracedCharacters.IsEmpty())
	{
		OnShotgunHitConfirmed.Broadcast(Result);
	}
}

FRewindResult ULagCompensationComponent::HitScanRewind(const FHitScanRewindRequest& Request)
{
	const auto& [BaseRequest, HitLocation] = Request;
	const auto& [HitCharacter, TraceStart, HitTime] = BaseRequest;

	if (not IsValid(HitCharacter))
		return {};

	if (const auto& FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
		FrameToCheck.IsSet())
	{
		auto TraceFunction = [&, this]()
		{
			return HitScanTrace(TraceStart, HitLocation);
		};
		return ConfirmHit(FrameToCheck.GetValue(), HitCharacter, TraceFunction);
	}
	return {};
}

FRewindResult ULagCompensationComponent::ProjectileRewind(const FProjectileRewindRequest& Request)
{
	const auto& [BaseRequest, InitialSpeed] = Request;
	const auto& [HitCharacter, TraceStart, HitTime] = BaseRequest;

	if (not IsValid(HitCharacter))
		return {};

	if (const auto& FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
		FrameToCheck.IsSet())
	{
		auto TraceFunction = [&, this]()
		{
			return ProjectileTrace(TraceStart, InitialSpeed);
		};
		return ConfirmHit(FrameToCheck.GetValue(), HitCharacter, TraceFunction);
	}
	return {};
}

FShotgunRewindResult ULagCompensationComponent::ShotgunRewind(
	const FShotgunRewindRequest& Request)
{
	const auto& [Requests] = Request;

	FShotgunRewindResult Result {};

	TMap<ABlasterCharacter*, FFramePackage> CharacterFrames;

	for (const auto& [BaseRequest, HitLocation] : Requests)
	{
		const auto& [HitCharacter, TraceStart, HitTime] = BaseRequest;

		if (not CharacterFrames.Contains(HitCharacter))
		{
			if (const auto& Frame = GetFrameToCheck(HitCharacter, HitTime);
				Frame.IsSet())
			{
				CharacterFrames.Add(HitCharacter, Frame.GetValue());
			}
		}
		auto&& FrameToCheck = CharacterFrames[HitCharacter];
		auto TraceFunction = [&, this]()
		{
			return HitScanTrace(TraceStart, HitLocation);
		};

		auto&& TmpResult = ConfirmHit(FrameToCheck, HitCharacter, TraceFunction);

		if (not Result.TracedCharacters.Contains(HitCharacter))
		{
			Result.TracedCharacters.Add(HitCharacter, {});
		}
		Result.TracedCharacters[HitCharacter].Results.Add(TmpResult);
	}

	return Result;
}

TOptional<FFramePackage> ULagCompensationComponent::GetFrameToCheck(const ABlasterCharacter* HitCharacter,
                                                                    float HitTime) const
{
	if (const auto* LagCompensationComponent = HitCharacter->GetComponentByClass<ULagCompensationComponent>();
		IsValid(LagCompensationComponent))
	{
		if (LagCompensationComponent->FrameHistory.GetHead() and LagCompensationComponent->FrameHistory.GetTail())
		{
			TOptional<FFramePackage> FrameToCheck {};
			const auto& History = LagCompensationComponent->FrameHistory;

			bool bShouldInterpolateFrame = true;

			if (const auto& [OldestTime, OldestFrame] = History.GetTail()->GetValue();
				HitTime < OldestTime) // Too far back to do server-side rewind
			{
				return {};
			}
			else if (HitTime == OldestTime)
			{
				FrameToCheck = History.GetTail()->GetValue();
				bShouldInterpolateFrame = false;
			}

			if (const auto& [NewestTime, NewestFrame] = History.GetHead()->GetValue();
				HitTime >= NewestTime)
			{
				FrameToCheck = History.GetHead()->GetValue();
				bShouldInterpolateFrame = false;
			}

			auto* YoungerIter = History.GetHead();
			auto* OlderIter = YoungerIter;

			while (OlderIter->GetValue().Time > HitTime)
			{
				if (not OlderIter->GetNextNode())
					break;

				OlderIter = OlderIter->GetNextNode();
				if (OlderIter->GetValue().Time > HitTime)
					YoungerIter = OlderIter;
			}

			if (OlderIter->GetValue().Time == HitTime) // Highly unlikely, but we found the frame to check
			{
				FrameToCheck = OlderIter->GetValue();
				bShouldInterpolateFrame = false;
			}

			if (bShouldInterpolateFrame)
			{
				// Interpolate the position between Younger and Older frames
				FrameToCheck = InterpBetweenFrames(OlderIter->GetValue(), YoungerIter->GetValue(), HitTime);
			}
			return FrameToCheck;
		}
	}
	return {};
}

TOptional<FHitResult> ULagCompensationComponent::HitScanTrace(const FVector_NetQuantize& TraceStart,
                                                              const FVector_NetQuantize& HitLocation) const
{
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	if (FHitResult ConfirmHitResult;
		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult,
		                                     TraceStart, TraceEnd,
		                                     ECC_HitBox))
	{
		return ConfirmHitResult;
	}
	return {};
}

TOptional<FHitResult> ULagCompensationComponent::ProjectileTrace(
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity) const
{
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = 4.f;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());

	/* DEBUG
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	*/

	if (FPredictProjectilePathResult PathResult;
		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult))
	{
		return PathResult.HitResult;
	}
	return {};
}

FRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
                                                    ABlasterCharacter* HitCharacter,
                                                    const TFunction<TOptional<FHitResult>()>& TraceFunction)
{
	FRewindResult Result {};

	FFramePackage CurrentFrame;
	CacheBoxPosition(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	for (auto& [Name, Box] : HitCharacter->HitBoxes)
	{
		if (not IsValid(Box))
			break;

		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
	}

	if (const auto& ConfirmHitResult = TraceFunction();
		ConfirmHitResult.IsSet())
	{
		if (ConfirmHitResult->bBlockingHit)
		{
			if (ConfirmHitResult->Component.IsValid())
			{
				for (auto& [Name, Box] : HitCharacter->HitBoxes)
				{
					if (Box == ConfirmHitResult->Component)
					{
						Result.TracedHitBoxes.Add(Name, ConfirmHitResult.GetValue());
					}
				}

				/* DEBUG
				if (auto* HitBox = Cast<UBoxComponent>(ConfirmHitResult->Component); 
					IsValid(HitBox))
				{
					DrawDebugBox(GetWorld(),
					             HitBox->GetComponentLocation(),
					             HitBox->GetScaledBoxExtent(),
					             FQuat { HitBox->GetComponentRotation() },
					             FColor::Red, false, 3.f);
				}
				*/
			}
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	return Result;
}

void ULagCompensationComponent::AddFrameToHistory()
{
	FFramePackage ThisFrame;
	SaveFramePackage(ThisFrame);
	FrameHistory.AddHead(ThisFrame);
}

float ULagCompensationComponent::GetHistoryLength() const
{
	const float HeadTime = FrameHistory.GetHead()->GetValue().Time;
	const float TailTime = FrameHistory.GetHead()->GetValue().Time;

	return HeadTime - TailTime;
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& Older,
                                                             const FFramePackage& Younger,
                                                             float HitTime) const
{
	const float Distance = Younger.Time - Older.Time;
	const float InterpFraction = FMath::Clamp((HitTime - Older.Time) / Distance, 0.f, 1.f);

	TMap<FName, FHitBoxInformation> InterpHitBoxes;

	for (auto& [Name, YoungerBox] : Younger.HitBoxInfo)
	{
		const FHitBoxInformation& OlderBox = Older.HitBoxInfo[Name];

		FHitBoxInformation InterpBoxInfo
		{
			.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction),
			.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction),
			.BoxExtent = YoungerBox.BoxExtent
		};

		InterpHitBoxes.Add(Name, InterpBoxInfo);
	}

	FFramePackage InterpFramePackage {
		.Time = HitTime,
		.HitBoxInfo = InterpHitBoxes
	};

	return InterpFramePackage;
}

void ULagCompensationComponent::CacheBoxPosition(ABlasterCharacter* Character, FFramePackage& OutFramePackage)
{
	if (not IsValid(Character))
		return;

	for (const auto& [Name, Box] : Character->HitBoxes)
	{
		if (IsValid(Box))
		{
			FHitBoxInformation BoxInfo {
				.Location = Box->GetComponentLocation(),
				.Rotation = Box->GetComponentRotation(),
				.BoxExtent = Box->GetScaledBoxExtent(),
			};
			OutFramePackage.HitBoxInfo.Add(Name, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* Character, const FFramePackage& TargetPackage)
{
	if (not IsValid(Character))
		return;

	for (auto& [Name, Box] : Character->HitBoxes)
	{
		if (IsValid(Box))
		{
			Box->SetWorldLocation(TargetPackage.HitBoxInfo[Name].Location);
			Box->SetWorldRotation(TargetPackage.HitBoxInfo[Name].Rotation);
			Box->SetBoxExtent(TargetPackage.HitBoxInfo[Name].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* Character, const FFramePackage& TargetPackage)
{
	if (not IsValid(Character))
		return;

	for (auto& [Name, Box] : Character->HitBoxes)
	{
		if (IsValid(Box))
		{
			Box->SetWorldLocation(TargetPackage.HitBoxInfo[Name].Location);
			Box->SetWorldRotation(TargetPackage.HitBoxInfo[Name].Rotation);
			Box->SetBoxExtent(TargetPackage.HitBoxInfo[Name].BoxExtent);
			Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* Character,
                                                             ECollisionEnabled::Type CollisionEnabled) const
{
	if (IsValid(Character) and IsValid(Character->GetMesh()))
	{
		Character->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

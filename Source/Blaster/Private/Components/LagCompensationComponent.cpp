// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"

#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"


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

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(const FServerSideRewindRequest& Request)
{
	const auto& [HitCharacter, TraceStart, HitLocation, HitTime] = Request;

	if (not IsValid(HitCharacter))
		return {};

	if (const auto& FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
		FrameToCheck.IsSet())
	{
		return ConfirmHit(FrameToCheck.GetValue(), HitCharacter, TraceStart, HitLocation);
	}
	return {};
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const FShotgunServerSideRewindRequest& Request)
{
	const auto& [Requests] = Request;

	FShotgunServerSideRewindResult Result {};

	TMap<ABlasterCharacter*, FFramePackage> CharacterFrames;

	for (const auto& [HitCharacter, TraceStart, HitLocation, HitTime] : Requests)
	{
		if (not CharacterFrames.Contains(HitCharacter))
		{
			if (const auto& Frame = GetFrameToCheck(HitCharacter, HitTime);
				Frame.IsSet())
			{
				CharacterFrames.Add(HitCharacter, Frame.GetValue());
			}
		}
		auto&& FrameToCheck = CharacterFrames[HitCharacter];
		auto&& TmpResult = ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);

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

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
                                                              ABlasterCharacter* HitCharacter,
                                                              const FVector_NetQuantize& TraceStart,
                                                              const FVector_NetQuantize& HitLocation)
{
	FServerSideRewindResult Result {};

	FFramePackage CurrentFrame;
	CacheBoxPosition(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

	for (auto& [Name, Box] : HitCharacter->HitBoxes)
	{
		if (not IsValid(Box))
			break;

		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

		if (FHitResult ConfirmHitResult;
			GetWorld()->LineTraceSingleByChannel(ConfirmHitResult,
			                                     TraceStart, TraceEnd,
			                                     ECC_Visibility))
		{
			if (ConfirmHitResult.bBlockingHit)
			{
				Result.TracedHitBoxes.Add(Name, ConfirmHitResult);
			}
		}
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

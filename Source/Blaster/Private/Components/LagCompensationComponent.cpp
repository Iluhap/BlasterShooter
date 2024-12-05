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
	ShowFramePackage(Package);
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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
		ShowFramePackage(FrameHistory.GetHead()->GetValue());
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
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

void ULagCompensationComponent::ServerSideRewind(const ABlasterCharacter* HitCharacter,
                                                 const FVector_NetQuantize& TraceStart,
                                                 const FVector_NetQuantize& HitTarget,
                                                 float HitTime)
{
	if (not IsValid(HitCharacter))
		return;

	if (const auto* LagCompensationComponent = HitCharacter->GetComponentByClass<ULagCompensationComponent>();
		IsValid(LagCompensationComponent))
	{
		if (LagCompensationComponent->FrameHistory.GetHead()
			and LagCompensationComponent->FrameHistory.GetTail())
		{
			FFramePackage FrameToCheck;
			const auto& History = LagCompensationComponent->FrameHistory;

			bool bShouldInterpolateFrame = true;

			if (const auto& [OldestTime, OldestFrame] = History.GetTail()->GetValue();
				HitTime < OldestTime) // Too far back to do server-side rewind
			{
				return;
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
			}
		}
	}
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
                                                             float HitTime)
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

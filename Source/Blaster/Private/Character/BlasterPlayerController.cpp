// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterPlayerController.h"

#include "EnhancedInputSubsystems.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (const auto* LocalPlayer = GetLocalPlayer())
	{
		if (auto* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}
}

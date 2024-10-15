// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"


void UOverheadWidget::SetDisplayText(const FString& TextToDisplay)
{
	if (IsValid(DisplayText))
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* Pawn)
{
	const ENetRole RemoteRole = Pawn->GetRemoteRole();
	const ENetRole LocalRole = Pawn->GetLocalRole();

	const FString RoleString = FString::Printf(TEXT("LocalRole: %s, Remote Role: %s"),
	                                                 *NetRoleToString(LocalRole), *NetRoleToString(RemoteRole));
	SetDisplayText(RoleString);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}

FString UOverheadWidget::NetRoleToString(const ENetRole& Role)
{
	switch (Role)
	{
	case ROLE_Authority:
		return "Authority";
	case ROLE_AutonomousProxy:
		return "Autonomous Proxy";
	case ROLE_SimulatedProxy:
		return "Simulated Proxy";
	default:
		return "None";
	}
}

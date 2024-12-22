// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"


USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UTexture2D* CrosshairCenter;

	UPROPERTY()
	UTexture2D* CrosshairTop;

	UPROPERTY()
	UTexture2D* CrosshairBottom;

	UPROPERTY()
	UTexture2D* CrosshairRight;

	UPROPERTY()
	UTexture2D* CrosshairLeft;

	UPROPERTY()
	float CrosshairSpread;

	UPROPERTY()
	FLinearColor CrosshairColor;
};

UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void SetHUDPackage(const FHUDPackage& NewHUDPackage) { HUDPackage = NewHUDPackage; };

	void AddCharacterOverlay();
	void AddAnnouncement();

protected:
	virtual void BeginPlay() override;

private:
	void DrawCrosshair(UTexture2D* Texture,
	                   const FVector2D& ViewPortCenter, const FVector2D& Spread,
	                   const FLinearColor& CrosshairColor);

private:
	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere, Category="Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category="Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;

public:
	UPROPERTY()
	TObjectPtr<class UCharacterOverlay> CharacterOverlay;

	UPROPERTY()
	TObjectPtr<class UAnnouncement> Announcement;
};

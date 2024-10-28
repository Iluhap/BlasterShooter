// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	if (GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter { ViewportSize.X / 2.f, ViewportSize.Y / 2.f };

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairCenter)
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, {}, HUDPackage.CrosshairColor);

		if (HUDPackage.CrosshairTop)
			DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, { 0.f, -SpreadScaled }, HUDPackage.CrosshairColor);

		if (HUDPackage.CrosshairBottom)
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, { 0, SpreadScaled }, HUDPackage.CrosshairColor);

		if (HUDPackage.CrosshairRight)
			DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, { SpreadScaled, 0.f }, HUDPackage.CrosshairColor);

		if (HUDPackage.CrosshairLeft)
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, { -SpreadScaled, 0.f }, HUDPackage.CrosshairColor);
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture,
                                const FVector2D& ViewPortCenter, const FVector2D& Spread,
                                const FLinearColor& CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D TextureDrawPoint {
		ViewPortCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewPortCenter.Y - (TextureHeight / 2.f) + Spread.Y
	};

	DrawTexture(Texture,
	            TextureDrawPoint.X, TextureDrawPoint.Y,
	            TextureWidth, TextureHeight,
	            0.f, 0.f,
	            1.f, 1.f,
	            CrosshairColor
	);
}

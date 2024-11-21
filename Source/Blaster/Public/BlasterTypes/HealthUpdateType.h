#pragma once

UENUM(BlueprintType)
enum class EHealthUpdateType : uint8
{
	EHU_Init UMETA(DisplayName = "Init"),
	EHU_Healing UMETA(DisplayName = "Healing"),
	EHU_Damage UMETA(DisplayName = "Damage"),

	EHU_MAX UMETA(DisplayName = "Default MAX"),
};

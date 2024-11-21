// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"

#include "Net/UnrealNetwork.h"


UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MaxHealth = 100.f;
	Health = MaxHealth;

	LastDamageInstigator = nullptr;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, MaxHealth);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::OnTakeDamage);
	}

	Health = MaxHealth;
	OnHealthUpdate.Broadcast(Health, MaxHealth, EHealthUpdateType::EHU_Healing);
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealTick(DeltaTime);
}

void UHealthComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	AmountToHeal = HealAmount;
	HealingRate = HealAmount / HealingTime;
}

void UHealthComponent::HealTick(float DeltaTime)
{
	if (not bHealing || IsDead())
		return;

	const float HealThisFrame = HealingRate * DeltaTime;

	Health = FMath::Clamp(Health + HealThisFrame, 0, MaxHealth);
	AmountToHeal -= HealThisFrame;

	OnHealthUpdate.Broadcast(Health, MaxHealth, EHealthUpdateType::EHU_Healing);

	if (Health >= MaxHealth)
	{
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UHealthComponent::OnTakeDamage(AActor* DamagedActor,
                                    float Damage, const UDamageType* DamageType,
                                    AController* InstigatedBy, AActor* DamageCauser)
{
	if (IsDead())
		return;

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	OnHealthUpdate.Broadcast(Health, MaxHealth, EHealthUpdateType::EHU_Damage);

	LastDamageInstigator = InstigatedBy;

	if (GetOwner()->HasAuthority())
	{
		if (IsDead())
		{
			OnDeath.Broadcast(InstigatedBy);
		}
	}
}

void UHealthComponent::OnRep_Health(float LastHealth)
{
	const EHealthUpdateType UpdateType = Health >= LastHealth ? EHealthUpdateType::EHU_Healing : EHealthUpdateType::EHU_Damage;
	OnHealthUpdate.Broadcast(Health, MaxHealth, UpdateType);
}

void UHealthComponent::OnRep_MaxHealth()
{
	OnHealthUpdate.Broadcast(Health, MaxHealth, EHealthUpdateType::EHU_Init);
}

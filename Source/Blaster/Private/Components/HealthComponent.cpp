// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"

#include "Net/UnrealNetwork.h"


UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	MaxHealth = 100.f;
	Health = MaxHealth;

	bHealing = false;
	HealingRate = 0.f;
	AmountToHeal = 0.f;

	bRestoringShield = false;
	ShieldRestorationRate = MaxShield / ShieldRestorationTime;
	AmountToHeal = 0.f;

	MaxShield = 100.f;
	Shield = 0.f;

	ShieldReductionTime = 30.f;
	ShieldReductionRate = MaxShield / ShieldReductionTime;

	LastDamageInstigator = nullptr;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, MaxHealth);
	DOREPLIFETIME(UHealthComponent, Health);

	DOREPLIFETIME(UHealthComponent, MaxShield);
	DOREPLIFETIME(UHealthComponent, Shield);
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

	RestoreShield(MaxShield);
	OnShieldUpdate.Broadcast(Shield, MaxShield);
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealTick(DeltaTime);
	ShieldTick(DeltaTime);
}

void UHealthComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	AmountToHeal = HealAmount;
	HealingRate = HealAmount / HealingTime;
}

void UHealthComponent::RestoreShield(float RestoreAmount)
{
	Shield = FMath::Clamp(Shield + RestoreAmount, 0, MaxShield);
	OnShieldUpdate.Broadcast(Shield, MaxShield);
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

void UHealthComponent::ShieldTick(float DeltaTime)
{
	if (IsDead() or Shield <= 0)
		return;

	const float ReduceShieldThisFrame = ShieldReductionRate * DeltaTime;
	ReduceShield(ReduceShieldThisFrame);
}

void UHealthComponent::ReduceHealth(float Damage)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	OnHealthUpdate.Broadcast(Health, MaxHealth, EHealthUpdateType::EHU_Damage);
}

void UHealthComponent::ReduceShield(float ReduceAmount)
{
	Shield = FMath::Clamp(Shield - ReduceAmount, 0, MaxShield);
	OnShieldUpdate.Broadcast(Shield, MaxShield);
}

void UHealthComponent::OnTakeDamage(AActor* DamagedActor,
                                    float Damage, const UDamageType* DamageType,
                                    AController* InstigatedBy, AActor* DamageCauser)
{
	if (IsDead())
		return;

	LastDamageInstigator = InstigatedBy;

	// Shield breaks without damaging the health even if damage was higher than shield amount
	if (IsShieldActive())
	{
		ReduceShield(Damage);
		return;
	}

	ReduceHealth(Damage);

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
	const EHealthUpdateType UpdateType = Health >= LastHealth
		                                     ? EHealthUpdateType::EHU_Healing
		                                     : EHealthUpdateType::EHU_Damage;
	OnHealthUpdate.Broadcast(Health, MaxHealth, UpdateType);
}

void UHealthComponent::OnRep_MaxHealth()
{
	OnHealthUpdate.Broadcast(Health, MaxHealth, EHealthUpdateType::EHU_Init);
}

void UHealthComponent::OnRep_Shield()
{
	OnShieldUpdate.Broadcast(Shield, MaxShield);
}

void UHealthComponent::OnRep_MaxShield()
{
	OnShieldUpdate.Broadcast(Shield, MaxShield);
}

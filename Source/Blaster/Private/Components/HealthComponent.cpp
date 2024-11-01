// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"

#include "Net/UnrealNetwork.h"


UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MaxHealth = 100.f;
	Health = MaxHealth;

	LastDamageInstigator = nullptr;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::OnTakeDamage);

	Health = MaxHealth;
	OnHealthUpdate.Broadcast(Health, MaxHealth);
}

void UHealthComponent::OnTakeDamage(AActor* DamagedActor,
                                    float Damage, const UDamageType* DamageType,
                                    AController* InstigatedBy, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	OnHealthUpdate.Broadcast(Health, MaxHealth);

	LastDamageInstigator = InstigatedBy;

	if (GetOwner()->HasAuthority())
	{
		if (Health <= 0.f)
		{
			OnDeath.Broadcast(InstigatedBy);
		}
	}
}

void UHealthComponent::OnRep_Health()
{
	UE_LOG(LogTemp, Warning, TEXT("Health replicated Health: %f"), Health);
	OnHealthUpdate.Broadcast(Health, MaxHealth);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/AmmoCasing.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"


AAmmoCasing::AAmmoCasing()
{
	PrimaryActorTick.bCanEverTick = false;

	DestroyDelay = 5.f;
	EjectionImpulse = 10.f;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);

	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
}

void AAmmoCasing::BeginPlay()
{
	Super::BeginPlay();

	Mesh->OnComponentHit.AddDynamic(this, &AAmmoCasing::OnHit);

	Mesh->AddImpulse(GetActorForwardVector() * EjectionImpulse);
}

void AAmmoCasing::PerformDestroy()
{
	Destroy();
}

void AAmmoCasing::OnHit(UPrimitiveComponent* HitComponent,
                        AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(DestroyTimerHandle))
		return;

	UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());

	GetWorld()->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		[this]()
		{
			Destroy();
		},
		DestroyDelay, false);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Components/CombatComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/AmmoCasing.h"


AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	AActor::SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon Mesh");
	SetRootComponent(Mesh);

	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetMeshOutlineColor(CUSTOM_DEPTH_BLUE);
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>("Area Sphere");
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>("Pick Widget");
	PickupWidget->SetupAttachment(RootComponent);

	FireRate = 600.f;
	bAutomatic = false;

	MagazineCapacity = 30;
	Ammo = MagazineCapacity;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	State = EWeaponState::EWS_Initial;

	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnAreaBeginOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnAreaEndOverlap);
	}

	if (IsValid(PickupWidget))
	{
		PickupWidget->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, State);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                 bool bFromSweep, const FHitResult& SweepResult)
{
	if (auto* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
		IsValid(BlasterCharacter))
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent,
                               AActor* OtherActor, UPrimitiveComponent* OtherComp,
                               int32 OtherBodyIndex)
{
	if (auto* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
		IsValid(BlasterCharacter))
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (IsValid(PickupWidget))
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::SetState(const EWeaponState NewState)
{
	State = NewState;

	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (State)
	{
	case EWeaponState::EWS_Equipped:
		{
			OnEquipped();
			break;
		}
	case EWeaponState::EWS_Equipped_Secondary:
		{
			OnEquippedSecondary();
			break;
		}
	case EWeaponState::EWS_Dropped:
		{
			OnDropped();
			break;
		}
	default: break;
	}
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetMeshCollision(false);
	EnableCustomDepth(false);
}

void AWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetMeshCollision(false);
	EnableCustomDepth(true);
	SetMeshOutlineColor(CUSTOM_DEPTH_TAN);
}

void AWeapon::OnDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		if (bDestroyOnDrop)
			Destroy();
	}
	SetMeshCollision(true);

	SetMeshOutlineColor(CUSTOM_DEPTH_BLUE);
	EnableCustomDepth(true);
}

void AWeapon::SetDestroyOnDrop(bool bDestroy)
{
	bDestroyOnDrop = bDestroy;
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (IsValid(FireAnimation))
	{
		Mesh->PlayAnimation(FireAnimation, false);
	}

	if (IsValid(AmmoCasingClass))
	{
		if (const auto* AmmoEjectSocket = Mesh->GetSocketByName(FName { "AmmoEject" });
			IsValid(AmmoEjectSocket))
		{
			const auto SocketTransform = AmmoEjectSocket->GetSocketTransform(Mesh);

			GetWorld()->SpawnActor<AAmmoCasing>(
				AmmoCasingClass,
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator()
			);
		}
	}

	SpendRound();
}

void AWeapon::Dropped()
{
	SetState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules { EDetachmentRule::KeepWorld, true };
	Mesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);

	OwningBlasterCharacter = nullptr;
	OwningBlasterPlayerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo - AmmoToAdd, 0, MagazineCapacity);
	UpdateHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (not IsValid(Owner))
	{
		OwningBlasterCharacter = nullptr;
		OwningBlasterPlayerController = nullptr;
	}

	if (IsValid(Owner))
	{
		UpdateHUDAmmo();
	}
}

void AWeapon::SetMeshCollision(bool bEnable)
{
	const auto CollisionEnabled = bEnable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;

	Mesh->SetCollisionEnabled(CollisionEnabled);

	Mesh->SetSimulatePhysics(bEnable);
	Mesh->SetEnableGravity(bEnable);

	if (bEnable)
	{
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	else
	{
		if (GetWeaponType() == EWeaponType::EWT_SubmachineGun)
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Mesh->SetEnableGravity(true);
			Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagazineCapacity);

	UpdateHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
	UpdateHUDAmmo();

	if (IsValid(OwningBlasterCharacter))
	{
		if (auto* CombatComponent = OwningBlasterCharacter->FindComponentByClass<UCombatComponent>();
			IsValid(CombatComponent) and IsFull())
		{
			CombatComponent->JumpToShotgunEnd();
		}
	}
}

void AWeapon::UpdateHUDAmmo()
{
	SetOwningController();
	if (IsValid(OwningBlasterPlayerController))
	{
		OwningBlasterPlayerController->SetHUDWeaponAmmo(Ammo);
	}
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (IsValid(Mesh))
	{
		Mesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::SetMeshOutlineColor(int32 DepthValue)
{
	if (IsValid(Mesh))
	{
		Mesh->SetCustomDepthStencilValue(DepthValue);
		Mesh->MarkRenderStateDirty();
	}
}

bool AWeapon::IsEmpty() const
{
	return Ammo <= 0;
}

bool AWeapon::IsFull() const
{
	return Ammo == MagazineCapacity;
}

void AWeapon::SetOwningCharacter()
{
	OwningBlasterCharacter = not IsValid(OwningBlasterCharacter)
		                         ? Cast<ABlasterCharacter>(GetOwner())
		                         : OwningBlasterCharacter;
}

void AWeapon::SetOwningController()
{
	SetOwningCharacter();

	if (not IsValid(OwningBlasterCharacter))
		return;

	OwningBlasterPlayerController = not IsValid(OwningBlasterPlayerController)
		                                ? Cast<ABlasterPlayerController>(OwningBlasterCharacter->Controller)
		                                : OwningBlasterPlayerController;
}

void AWeapon::OnRep_State()
{
	OnWeaponStateSet();
}

#include "Character/BlasterCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Blaster/Blaster.h"
#include "Character/BlasterPlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/LagCompensationComponent.h"
#include "Components/LeaderCrownComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameMode/BlasterGameMode.h"
#include "GameState/BlasterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Weapon/Weapon.h"
#include "Weapon/WeaponTypes.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CameraArm = CreateDefaultSubobject<USpringArmComponent>("Camera Arm");
	CameraArm->SetupAttachment(GetMesh());
	CameraArm->TargetArmLength = 300.f;
	CameraArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("Follow Camera");
	FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>("Overhead Widget");
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>("Combat Component");
	Combat->SetIsReplicated(true);

	Health = CreateDefaultSubobject<UHealthComponent>("Health Component");
	Health->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>("Buff Component");
	Buff->SetIsReplicated(true);

	LagCompensationComponent = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	LeaderCrownComponent = CreateDefaultSubobject<ULeaderCrownComponent>(TEXT("Leader Crown Component"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>("Grenade Mesh");
	GrenadeMesh->SetupAttachment(GetMesh(), FName { "RightHandSocket" });
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrenadeMesh->SetVisibility(false);

	CameraThreshold = 200.f;

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	TurnThreshold = 0.5f;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	EliminationDelay = 1.5f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>("DissolveTimeline");
	BaseDissolveValue = 0.55;
	BaseDissolveGlow = 200.f;

	bLeftGame = false;

	InitHitBoxes();
}

void ABlasterCharacter::InitHitBoxes()
{
	Head = CreateDefaultSubobject<UBoxComponent>("Head");
	Head->SetupAttachment(GetMesh(), FName("head"));

	HitBoxes.Add(FName { "Head" }, Head);

	Pelvis = CreateDefaultSubobject<UBoxComponent>("Pelvis");
	Pelvis->SetupAttachment(GetMesh(), FName("pelvis"));

	HitBoxes.Add(FName { "Pelvis" }, Pelvis);

	Spine_02 = CreateDefaultSubobject<UBoxComponent>("Spine_02");
	Spine_02->SetupAttachment(GetMesh(), FName("spine_02"));

	HitBoxes.Add(FName { "Spine_02" }, Spine_02);

	Spine_03 = CreateDefaultSubobject<UBoxComponent>("Spine_03");
	Spine_03->SetupAttachment(GetMesh(), FName("spine_03"));

	HitBoxes.Add(FName { "Spine_03" }, Spine_03);

	UpperArm_L = CreateDefaultSubobject<UBoxComponent>("UpperArm_L");
	UpperArm_L->SetupAttachment(GetMesh(), FName("upperarm_l"));

	HitBoxes.Add(FName { "UpperArm_L" }, UpperArm_L);

	UpperArm_R = CreateDefaultSubobject<UBoxComponent>("UpperArm_R");
	UpperArm_R->SetupAttachment(GetMesh(), FName("upperarm_r"));

	HitBoxes.Add(FName { "UpperArm_R" }, UpperArm_R);

	LowerArm_L = CreateDefaultSubobject<UBoxComponent>("LowerArm_L");
	LowerArm_L->SetupAttachment(GetMesh(), FName("lowerarm_l"));

	HitBoxes.Add(FName { "LowerArm_L" }, LowerArm_L);

	LowerArm_R = CreateDefaultSubobject<UBoxComponent>("LowerArm_R");
	LowerArm_R->SetupAttachment(GetMesh(), FName("lowerarm_r"));

	HitBoxes.Add(FName { "LowerArm_R" }, LowerArm_R);

	Hand_L = CreateDefaultSubobject<UBoxComponent>("Hand_L");
	Hand_L->SetupAttachment(GetMesh(), FName("hand_l"));

	HitBoxes.Add(FName { "Hand_L" }, Hand_L);

	Hand_R = CreateDefaultSubobject<UBoxComponent>("Hand_R");
	Hand_R->SetupAttachment(GetMesh(), FName("hand_r"));
	HitBoxes.Add(FName { "Hand_R" }, Hand_R);

	Thigh_L = CreateDefaultSubobject<UBoxComponent>("Thigh_L");
	Thigh_L->SetupAttachment(GetMesh(), FName("thigh_l"));

	HitBoxes.Add(FName { "Thigh_L" }, Thigh_L);

	Thigh_R = CreateDefaultSubobject<UBoxComponent>("Thigh_R");
	Thigh_R->SetupAttachment(GetMesh(), FName("thigh_r"));

	HitBoxes.Add(FName { "Thigh_R" }, Thigh_R);

	Calf_L = CreateDefaultSubobject<UBoxComponent>("Calf_L");
	Calf_L->SetupAttachment(GetMesh(), FName("calf_l"));

	HitBoxes.Add(FName { "Calf_L" }, Calf_L);

	Calf_R = CreateDefaultSubobject<UBoxComponent>("Calf_R");
	Calf_R->SetupAttachment(GetMesh(), FName("calf_r"));

	HitBoxes.Add(FName { "Calf_R" }, Calf_R);

	Foot_L = CreateDefaultSubobject<UBoxComponent>("Foot_L");
	Foot_L->SetupAttachment(GetMesh(), FName("foot_l"));
	HitBoxes.Add(FName { "Foot_L" }, Foot_L);

	Foot_R = CreateDefaultSubobject<UBoxComponent>("Foot_R");
	Foot_R->SetupAttachment(GetMesh(), FName("foot_r"));

	HitBoxes.Add(FName { "Foot_R" }, Foot_R);

	for (auto& [_, HitBox] : HitBoxes)
	{
		if (IsValid(HitBox))
		{
			HitBox->SetCollisionObjectType(ECC_HitBox);
			HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
			HitBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
			HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (not IsValid(Input))
		return;

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ABlasterCharacter::Jump);
	Input->BindAction(EquipAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Equip);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABlasterCharacter::OnCrouch);

	Input->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::StartAim);
	Input->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::StopAim);

	Input->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::StartFire);
	Input->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::StopFire);

	Input->BindAction(ReloadAction, ETriggerEvent::Completed, this, &ABlasterCharacter::Reload);

	Input->BindAction(ThrowGrenadeAction, ETriggerEvent::Started, this, &ABlasterCharacter::ThrowGrenade);

	Input->BindAction(SwapWeaponsAction, ETriggerEvent::Completed, this, &ABlasterCharacter::SwapWeapons);
}

void ABlasterCharacter::DisableGameplay()
{
	bDisableGameplay = true;

	Combat->SetFiring(false);
}

void ABlasterCharacter::GainedTheLead()
{
	if (IsValid(LeaderCrownComponent))
	{
		LeaderCrownComponent->MulticastSpawnCrown();
	}
}

void ABlasterCharacter::LostTheLead()
{
	if (IsValid(LeaderCrownComponent))
	{
		LeaderCrownComponent->MulticastRemoveCrown();
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Combat))
	{
		Combat->SpawnDefaultWeapon();
	}

	UpdateHUDHealth();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
		Health->OnDeath.AddDynamic(this, &ABlasterCharacter::OnDeath);
	}
	Health->OnHealthUpdate.AddDynamic(this, &ABlasterCharacter::OnHealthUpdate);
	Health->OnShieldUpdate.AddDynamic(this, &ABlasterCharacter::OnShieldUpdate);
}

void ABlasterCharacter::PollInit()
{
	if (not IsValid(BlasterPlayerState))
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (IsValid(BlasterPlayerState))
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0.f);

			if (const auto* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
				IsValid(BlasterGameState)
				and BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
			{
				if (IsValid(LeaderCrownComponent))
					LeaderCrownComponent->MulticastSpawnCrown();
			}
		}
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (GetLocalRole() > ROLE_SimulatedProxy and IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementRep += DeltaTime;
		if (TimeSinceLastMovementRep > 0.25f)
			OnRep_ReplicateMovement();

		CalculateAimOffsetPitch();
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PollInit();

	RotateInPlace(DeltaTime);

	HideCharacterIfCameraClose();
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay)
		return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
}

void ABlasterCharacter::PlayFireMontage(bool IsAiming) const
{
	if (not IsValid(Combat) or not Combat->IsWeaponEquipped())
		return;

	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(FireWeaponMontage))
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = IsAiming ? FName { "RifleAim" } : FName { "RifleHip" };

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage() const
{
	if (not IsValid(Combat) or not Combat->IsWeaponEquipped())
		return;

	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(ReloadMontage))
	{
		FName SectionName;

		switch (Combat->GetEquippedWeapon()->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			{
				SectionName = FName("Rifle");
				break;
			}
		case EWeaponType::EWT_RocketLauncher:
			{
				SectionName = FName("RocketLauncher");
				break;
			}
		case EWeaponType::EWT_Pistol:
			{
				SectionName = FName("Pistol");
				break;
			}
		case EWeaponType::EWT_SubmachineGun:
			{
				SectionName = FName("Pistol");
				break;
			}
		case EWeaponType::EWT_Shotgun:
			{
				SectionName = FName("Shotgun");
				break;
			}
		case EWeaponType::EWT_SniperRifle:
			{
				SectionName = FName("SniperRifle");
				break;
			}
		case EWeaponType::EWT_GrenadeLauncher:
			{
				SectionName = FName("GrenadeLauncher");
				break;
			}
		default:
			{
				break;
			}
		}
		AnimInstance->Montage_Play(ReloadMontage);
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayEliminationMontage() const
{
	const TArray<FName> SectionNames = { "Elim1", "Elim2", "Elim3" };

	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(EliminationMontage))
	{
		AnimInstance->Montage_Play(EliminationMontage);

		const FName SectionName = SectionNames[FMath::Rand32() % 3];
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage() const
{
	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(ThrowGrenadeMontage))
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwapWeaponMontage() const
{
	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(SwapWeaponMontage))
	{
		AnimInstance->Montage_Play(SwapWeaponMontage);
	}
}

void ABlasterCharacter::Eliminate(bool bPlayerLeftGame)
{
	if (IsValid(Combat) and Combat->IsWeaponEquipped())
	{
		Combat->DropWeapons();
	}

	MulticastEliminate(bPlayerLeftGame);
}

void ABlasterCharacter::LeaveGame()
{
	ServerLeaveGame();
}

void ABlasterCharacter::MulticastEliminate_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	bIsEliminated = true;

	PlayEliminationMontage();
	StartDissolve();
	DisableMovement();

	if (IsValid(BlasterPlayerController))
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}

	const bool bHideSniperScope = IsLocallyControlled()
		and IsValid(Combat)
		and Combat->IsAiming()
		and Combat->IsWeaponEquipped()
		and Combat->GetEquippedWeapon()->GetWeaponType() == EWeaponType::EWT_SniperRifle;

	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}

	GetWorld()->GetTimerManager().SetTimer(EliminationTimerHandle,
	                                       this, &ABlasterCharacter::EliminationTimerFinished,
	                                       EliminationDelay);
}

void ABlasterCharacter::EliminationTimerFinished()
{
	if (auto* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		IsValid(BlasterGameMode) and not bLeftGame)
	{
		BlasterGameMode->RequestRespawn(this, GetController());
	}
	if (bLeftGame and IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	if (auto* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		IsValid(BlasterGameMode))
	{
		if (IsValid(BlasterPlayerState))
		{
			BlasterGameMode->PlayerLeftGame(this);
		}
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	SetDissolveParams(DissolveValue, BaseDissolveGlow);
}

void ABlasterCharacter::StartDissolve()
{
	if (IsValid(DissolveMaterialInstance))
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);

		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		GetMesh()->SetMaterial(1, DynamicDissolveMaterialInstance);

		SetDissolveParams(BaseDissolveValue, BaseDissolveGlow);
	}

	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (IsValid(DissolveCurve))
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetDissolveParams(const float& Dissolve, const float& Glow)
{
	DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), Dissolve);
	DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), Glow);
}

void ABlasterCharacter::DisableMovement()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	DisableGameplay();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::PlayHitReactMontage() const
{
	if (not IsValid(Combat) or not Combat->IsWeaponEquipped())
		return;

	if (auto* AnimInstance = GetMesh()->GetAnimInstance();
		IsValid(AnimInstance) and IsValid(HitReactMontage))
	{
		AnimInstance->Montage_Play(HitReactMontage);
		const FName SectionName = FName { "FromFront" };

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::CalculateAimOffsetPitch()
{
	AimOffsetPitch = GetBaseAimRotation().Pitch;

	if (AimOffsetPitch > 90.f and not IsLocallyControlled())
	{
		const FVector2D InRange { 270.f, 360.f };
		const FVector2D OutRange { -90.f, 0.f };

		AimOffsetPitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AimOffsetPitch);
	}
}

float ABlasterCharacter::GetSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                      AController* InstigatedBy, AActor* DamageCauser)
{
}

void ABlasterCharacter::OnDeath(AController* InstigatedBy)
{
	if (auto* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		IsValid(BlasterGameMode))
	{
		BlasterPlayerController = IsValid(BlasterPlayerController)
			                          ? Cast<ABlasterPlayerController>(GetController())
			                          : BlasterPlayerController;

		auto* AttackerController = Cast<ABlasterPlayerController>(InstigatedBy);

		BlasterGameMode->PlayerEliminated(this, BlasterPlayerController,
		                                  AttackerController);
	}
}

void ABlasterCharacter::OnHealthUpdate(const float& NewHealth, const float& NewMaxHealth, EHealthUpdateType UpdateType)
{
	if (UpdateType == EHealthUpdateType::EHU_Damage)
	{
		PlayHitReactMontage();
	}

	UpdateHUDHealth();
}

void ABlasterCharacter::OnShieldUpdate(const float& NewShield, const float& NewMaxShield)
{
	UpdateHUDShield();
}

void ABlasterCharacter::SetController()
{
	BlasterPlayerController = Cast<ABlasterPlayerController>(GetController());
}

void ABlasterCharacter::UpdateHUDHealth()
{
	SetController();

	if (not IsValid(BlasterPlayerController))
		return;

	BlasterPlayerController->SetHUDHealth(Health->GetHealth(), Health->GetMaxHealth());
}

void ABlasterCharacter::UpdateHUDShield()
{
	SetController();

	if (not IsValid(BlasterPlayerController))
		return;

	BlasterPlayerController->SetHUDShield(Health->GetShield(), Health->GetMaxShield());
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// For playing server only
	if (IsValid(OverlappingWeapon))
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	// For playing server only
	if (IsLocallyControlled())
	{
		if (IsValid(OverlappingWeapon))
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_ReplicateMovement()
{
	Super::OnRep_ReplicateMovement();

	SimProxiesTurn();

	TimeSinceLastMovementRep = 0;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (IsValid(OverlappingWeapon))
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (IsValid(LastWeapon))
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	if (bDisableGameplay)
		return;

	const FVector2D AxisValue = Value.Get<FVector2D>();

	if (IsValid(Controller))
	{
		const FRotator Yaw = { 0.f, Controller->GetControlRotation().Yaw, 0.f };
		const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, AxisValue.Y);
		AddMovementInput(Right, AxisValue.X);
	}
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D AxisValue = Value.Get<FVector2D>();
	AddControllerYawInput(AxisValue.X);
	AddControllerPitchInput(AxisValue.Y);
}

void ABlasterCharacter::Equip()
{
	if (bDisableGameplay)
		return;

	ServerEquip();
}

void ABlasterCharacter::ServerEquip_Implementation()
{
	if (IsValid(Combat))
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::OnCrouch()
{
	if (bDisableGameplay)
		return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::StartAim()
{
	if (bDisableGameplay)
		return;

	if (IsValid(Combat))
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::StopAim()
{
	if (bDisableGameplay)
		return;

	if (IsValid(Combat))
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::StartFire()
{
	if (bDisableGameplay)
		return;

	if (IsValid(Combat))
	{
		Combat->SetFiring(true);
	}
}

void ABlasterCharacter::StopFire()
{
	if (bDisableGameplay)
		return;

	if (IsValid(Combat))
	{
		Combat->SetFiring(false);
	}
}

void ABlasterCharacter::Reload()
{
	if (bDisableGameplay)
		return;

	if (IsValid(Combat))
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::ThrowGrenade()
{
	if (IsValid(Combat))
	{
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::SwapWeapons()
{
	PlaySwapWeaponMontage();

	ServerSwapWeapons();
}

void ABlasterCharacter::ServerSwapWeapons_Implementation()
{
	if (IsValid(Combat))
	{
		Combat->SwapWeapons();
	}
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat and Combat->IsWeaponEquipped();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat and not Combat->IsWeaponEquipped())
		return;

	const float Speed = GetSpeed();
	const bool IsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f and not IsInAir)
	{
		bRotateRootBone = true;

		const FRotator CurrentAimRotation { 0.f, GetBaseAimRotation().Yaw, 0.f };
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(
			CurrentAimRotation, StartingAimRotation);

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAimOffsetYaw = AimOffsetYaw;
		}

		AimOffsetYaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f or IsInAir)
	{
		bRotateRootBone = false;

		StartingAimRotation = { 0.f, GetBaseAimRotation().Yaw, 0.f };
		AimOffsetYaw = 0.f;
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAimOffsetPitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (not IsValid(Combat) or not Combat->IsWeaponEquipped())
		return;

	bRotateRootBone = false;

	if (GetSpeed() > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	CalculateAimOffsetPitch();

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();

	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AimOffsetYaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AimOffsetYaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAimOffsetYaw = FMath::FInterpTo(InterpAimOffsetYaw, 0.f, DeltaTime, 4.f);
		AimOffsetYaw = InterpAimOffsetYaw;

		if (FMath::Abs(AimOffsetYaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;

			StartingAimRotation = { 0.f, GetBaseAimRotation().Yaw, 0.f };
		}
	}
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (FVector::Distance(FollowCamera->GetComponentLocation(), GetActorLocation()) < CameraThreshold)
	{
		HideCharacter(true);
	}
	else
	{
		HideCharacter(false);
	}
}

void ABlasterCharacter::HideCharacter(bool bHide)
{
	if (not IsLocallyControlled())
		return;

	GetMesh()->SetVisibility(not bHide);

	if (IsValid(Combat) and Combat->IsWeaponEquipped())
	{
		if (auto* WeaponMesh = Combat->GetEquippedWeapon()->FindComponentByClass<USkeletalMeshComponent>();
			IsValid(WeaponMesh))
		{
			WeaponMesh->bOwnerNoSee = bHide;
		}
	}
}

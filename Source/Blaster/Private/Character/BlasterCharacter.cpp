 // Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Character/BlasterAnimInstance.h"
#include "Weapon/Weapon.h"
#include "BlasterComponents/CombatComponent.h"
#include "BlasterComponents/BuffComponent.h"
#include "Input/BlasterCharacterInputData.h"
#include "GameModes/BlasterGameMode.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Weapon/WeaponTypes.h"
#include "Enums/CombatState.h"

#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"

#include "Animation/AnimMontage.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"
#include "Particles/ParticleSystemComponent.h"

#include "Net/UnrealNetwork.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraBoom->bUsePawnControlRotation = true;

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;

    OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
    OverheadWidget->SetupAttachment(RootComponent);

    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
    CombatComponent->SetIsReplicated(true);

    BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
    BuffComponent->SetIsReplicated(true);

    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

    GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

    GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

    TurningInPlace = ETurningInPlace::ETIP_NotTurning;

    NetUpdateFrequency = 66.f;
    MinNetUpdateFrequency = 33.f;

    DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
    DOREPLIFETIME(ABlasterCharacter, Health);
    DOREPLIFETIME(ABlasterCharacter, Shield);
    DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (CombatComponent)
        CombatComponent->SetOwningCharacter(this);

    if (BuffComponent)
    {
        BuffComponent->SetOwningCharacter(this);

        BuffComponent->SetInitialSpeeds(
            GetCharacterMovement()->MaxWalkSpeed,
            GetCharacterMovement()->MaxWalkSpeedCrouched
        );

        BuffComponent->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
    }
}

void ABlasterCharacter::Destroyed()
{
    Super::Destroyed();

    if (EliminationBotComponent)
        EliminationBotComponent->DestroyComponent();

    ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

    if (CombatComponent && CombatComponent->GetEquippedWeapon() && bMatchNotInProgress)
        CombatComponent->GetEquippedWeapon()->Destroy();
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

    SpawnDefaultWeapon();

    UpdateHUDAmmo();
    UpdateHUDHealth();
    UpdateHUDShield();

    if (HasAuthority())
        OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
}


void ABlasterCharacter::PollInit()
{
    if (BlasterPlayerState == nullptr)
    {
        BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

        if (BlasterPlayerState)
        {
            BlasterPlayerState->AddToScore(0.f);
            BlasterPlayerState->AddToDefeats(0);
        }
    }
}


void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

    Subsystem->ClearAllMappings();
    Subsystem->AddMappingContext(InputMapping, 0);

    UEnhancedInputComponent* PlayerInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);

    PlayerInput->BindAction(InputActions->InputMove, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
    PlayerInput->BindAction(InputActions->InputLook, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
    PlayerInput->BindAction(InputActions->InputJump, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
    PlayerInput->BindAction(InputActions->InputEquip, ETriggerEvent::Triggered, this, &ABlasterCharacter::Equip);
    PlayerInput->BindAction(InputActions->InputCrouch, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchButtonPressed);
    PlayerInput->BindAction(InputActions->InputAim, ETriggerEvent::Started, this, &ABlasterCharacter::AimButtonPressed);
    PlayerInput->BindAction(InputActions->InputAim, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased);
    PlayerInput->BindAction(InputActions->InputFire, ETriggerEvent::Triggered, this, &ABlasterCharacter::FireButtonPressed);
    PlayerInput->BindAction(InputActions->InputFire, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased);
    PlayerInput->BindAction(InputActions->InputReload, ETriggerEvent::Triggered, this, &ABlasterCharacter::Reload);
}

void ABlasterCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    RotateInPlace(DeltaTime);

    HidePlayerIfTooClose();
    PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
    if (bDisableGameplay)
    {
        bUseControllerRotationYaw = false;
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;

        return;
    }

    if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
        AimOffset(DeltaTime);
    else
    {
        TimeSinceLastMovementReplication += DeltaTime;

        if (TimeSinceLastMovementReplication > .25f)
            OnRep_ReplicatedMovement();

        CalculateAO_Pitch();
    }
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (GetIsEliminated())
        return;

    float DamageToHealth = Shield > 0.f ? 0.f : Damage;

    if (Shield > 0.f)
    {
        float PreviousShield = Shield;
        Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);

        if (Shield == 0.f)
            DamageToHealth = FMath::Clamp(Damage - PreviousShield, 0.f, MaxHealth);
    }

    Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

    UpdateHUDHealth();
    UpdateHUDShield();
    PlayHitReactionMontage();

    if (Health == 0.f)
    {
        ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
        if (BlasterGameMode)
        {
            BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
            ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatedBy);

            BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
        }
    }
}

void ABlasterCharacter::Eliminated()
{
    if (CombatComponent && CombatComponent->GetEquippedWeapon())
    {
        if (CombatComponent->GetEquippedWeapon()->GetDestroyOnElimination())
            CombatComponent->GetEquippedWeapon()->Destroy();
        else
            CombatComponent->GetEquippedWeapon()->Drop();
    }

    Multicast_Eliminated();

    GetWorldTimerManager().SetTimer(
        EliminatedTimer,
        this,
        &ABlasterCharacter::EliminatedTimerFinished,
        EliminatedDelay
    );
}

void ABlasterCharacter::Multicast_Eliminated_Implementation()
{
    if (BlasterPlayerController)
        BlasterPlayerController->SetHUDWeaponAmmo(0);

    bIsEliminated = true;

    PlayEliminatedMontage();

    if (DissolveMaterialInstance)
    {
        DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
        GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);

        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), .55f);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 50.f);
    }

    StartDissolve();

    bDisableGameplay = true;
    GetCharacterMovement()->DisableMovement();

    if (CombatComponent)
        CombatComponent->FireButtonPressed(false);

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (EliminationBotEffect)
    {
        FVector EliminationBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);

        EliminationBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            EliminationBotEffect,
            EliminationBotSpawnPoint,
            GetActorRotation()
        );

        if (EliminationBotSound)
        {
            UGameplayStatics::SpawnSoundAtLocation(
                this,
                EliminationBotSound,
                GetActorLocation()
            );
        }
    }

    bool bHideSniperScope = IsLocallyControlled() &&
                            CombatComponent && CombatComponent->IsAiming() &&
                            CombatComponent->GetEquippedWeapon() && CombatComponent->GetEquippedWeapon()->GetWeaponType() == EWeaponType::EWT_Sniper;

    if (bHideSniperScope)
        ShowSniperScopeWidget(false);
}

void ABlasterCharacter::EliminatedTimerFinished()
{
    ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();

    if (BlasterGameMode)
        BlasterGameMode->RequestSpawn(this, Controller);
}

void ABlasterCharacter::StartDissolve()
{
    DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);

    if (DissolveCurve && DissolveTimeline)
    {
        DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
        DissolveTimeline->Play();
    }
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
    if (DynamicDissolveMaterialInstance)
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
    ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    UWorld* World = GetWorld();

    if (BlasterGameMode && World && DefaultWeaponClass && !GetIsEliminated())
    {
        AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
        DefaultWeapon->SetDestroyOnElimination(true);

        if (CombatComponent)
            CombatComponent->EquipWeapon(DefaultWeapon);
    }
}

void ABlasterCharacter::UpdateHUDHealth()
{
    BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

    if (BlasterPlayerController)
        BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
}

void ABlasterCharacter::UpdateHUDShield()
{
    BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

    if (BlasterPlayerController)
        BlasterPlayerController->SetHUDShield(Shield, MaxShield);
}

void ABlasterCharacter::UpdateHUDAmmo()
{
    BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

    if (BlasterPlayerController && CombatComponent && CombatComponent->GetEquippedWeapon())
    {
        BlasterPlayerController->SetHUDCarriedAmmo(CombatComponent->GetCarriedAmmo());
        BlasterPlayerController->SetHUDWeaponAmmo(CombatComponent->GetWeaponAmmo());
    }
}

void ABlasterCharacter::HidePlayerIfTooClose()
{
    if (!IsLocallyControlled()) return;

    if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
    {
        GetMesh()->SetVisibility(false);

        if (CombatComponent && CombatComponent->GetEquippedWeapon() && CombatComponent->GetEquippedWeapon()->GetWeaponMesh())
            CombatComponent->GetEquippedWeapon()->GetWeaponMesh()->bOwnerNoSee = true;
    }
    else
    {
        GetMesh()->SetVisibility(true);

        if (CombatComponent && CombatComponent->GetEquippedWeapon() && CombatComponent->GetEquippedWeapon()->GetWeaponMesh())
            CombatComponent->GetEquippedWeapon()->GetWeaponMesh()->bOwnerNoSee = false;
    }
}


void ABlasterCharacter::OnRep_ReplicatedMovement()
{
    Super::OnRep_ReplicatedMovement();

    SimProxiesTurn();

    TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (Controller != nullptr)
    {
        const FVector2D MoveValue = Value.Get<FVector2D>();
        const FRotator MovementRotation(0, Controller->GetControlRotation().Yaw, 0);

        if (MoveValue.Y != 0.f)
        {
            const FVector Direction = MovementRotation.RotateVector(FVector::ForwardVector);

            AddMovementInput(Direction, MoveValue.Y);
        }

        if (MoveValue.X != 0.f)
        {
            const FVector Direction = MovementRotation.RotateVector(FVector::RightVector);

            AddMovementInput(Direction, MoveValue.X);
        }
    }
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
    if (Controller != nullptr)
    {
        const FVector2D LookValue = Value.Get<FVector2D>();

        if (LookValue.X != 0.f)
        {
            AddControllerYawInput(LookValue.X);
        }

        if (LookValue.Y != 0.f)
        {
            AddControllerPitchInput(LookValue.Y);
        }
    }
}

void ABlasterCharacter::Equip(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (CombatComponent)
    {
        if (HasAuthority())
            CombatComponent->EquipWeapon(OverlappingWeapon);
        else
            Server_Equip();
    }
}

void ABlasterCharacter::Server_Equip_Implementation()
{
    CombatComponent->EquipWeapon(OverlappingWeapon);
}

void ABlasterCharacter::PlayFireMontage(bool bIsAiming)
{
    if (CombatComponent == nullptr || CombatComponent->GetEquippedWeapon() == nullptr) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && FireWeaponMontage)
    {
        AnimInstance->Montage_Play(FireWeaponMontage);

        FName SectionName;
        SectionName = bIsAiming ? FName("RifleAim") : FName("RifleHip");

        AnimInstance->Montage_JumpToSection(SectionName);
    }
}

void ABlasterCharacter::PlayReloadMontage()
{
    if (CombatComponent == nullptr || CombatComponent->GetEquippedWeapon() == nullptr) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && ReloadMontage)
    {
        AnimInstance->Montage_Play(ReloadMontage);

        FName SectionName;

        switch (CombatComponent->GetEquippedWeapon()->GetWeaponType())
        {
            case EWeaponType::EWT_AssaultRifle:
                SectionName = FName("Rifle");
                break;

            case EWeaponType::EWT_RocketLauncher:
                SectionName = FName("RocketLauncher");
                break;

            case EWeaponType::EWT_Pistol:
                SectionName = FName("Pistol");
                break;

            case EWeaponType::EWT_Shotgun:
                SectionName = FName("Shotgun");
                break;

            case EWeaponType::EWT_Sniper:
                SectionName = FName("Sniper");
                break;

            case EWeaponType::EWT_GrenadeLauncher:
                SectionName = FName("GrenadeLauncher");
                break;
        }

        AnimInstance->Montage_JumpToSection(SectionName);
    }
}

void ABlasterCharacter::PlayEliminatedMontage()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    if (AnimInstance && EliminatedMontage)
        AnimInstance->Montage_Play(EliminatedMontage);
}

void ABlasterCharacter::PlayHitReactionMontage()
{
    if (CombatComponent == nullptr || CombatComponent->GetEquippedWeapon() == nullptr) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && HitReactionMontage)
    {
        AnimInstance->Montage_Play(HitReactionMontage);

        FName SectionName = FName("FromFront");

        AnimInstance->Montage_JumpToSection(SectionName);
    }
}

void ABlasterCharacter::MulticastHitReaction_Implementation()
{
    PlayHitReactionMontage();
}

void ABlasterCharacter::CrouchButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (GetCharacterMovement()->IsCrouching())
        UnCrouch();
    else
        Crouch();
}

void ABlasterCharacter::AimButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (CombatComponent)
        CombatComponent->SetIsAiming(true);
}

void ABlasterCharacter::AimButtonReleased(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (CombatComponent)
        CombatComponent->SetIsAiming(false);
}

void ABlasterCharacter::FireButtonPressed(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (CombatComponent && CombatComponent->GetEquippedWeapon())
        CombatComponent->FireButtonPressed(true);
}

void ABlasterCharacter::FireButtonReleased(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (CombatComponent && CombatComponent->GetEquippedWeapon())
        CombatComponent->FireButtonPressed(false);
}

void ABlasterCharacter::Reload(const FInputActionValue& Value)
{
    if (bDisableGameplay)
        return;

    if (CombatComponent)
        CombatComponent->Reload();
}

float ABlasterCharacter::CalculateSpeed()
{
    FVector Velocity = GetVelocity();
    Velocity.Z = 0;

    return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
    if (CombatComponent && CombatComponent->GetEquippedWeapon() == nullptr) return;

    float Speed = CalculateSpeed();

    bool bIsInAir = GetCharacterMovement()->IsFalling();

    if (Speed == 0.f && !bIsInAir)
    {
        bRotateRootBone = true;

        FRotator CurrentRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation, StartingAimRotation);

        AO_Yaw = DeltaAimRotation.Yaw;
        bUseControllerRotationYaw = true;

        if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
            InterpAO_Yaw = AO_Yaw;

        TurnInPlace(DeltaTime);
    }
    else if (Speed > 0.f || bIsInAir)
    {
        bRotateRootBone = false;

        StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

        AO_Yaw = 0.f;
        bUseControllerRotationYaw = true;

        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    }

    CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
    AO_Pitch = GetBaseAimRotation().Pitch;

    if (AO_Pitch > 90.f && !IsLocallyControlled())
    {
        FVector2D InRange(270.f, 360.f);
        FVector2D OutRange(-90.f, 0.f);

        AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
    }
}

void ABlasterCharacter::SimProxiesTurn()
{
    if (CombatComponent == nullptr || CombatComponent->GetEquippedWeapon() == nullptr) return;

    bRotateRootBone = false;

    float Speed = CalculateSpeed();
    if (Speed > 0.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        return;
    }

    ProxyRotationLastFrame = ProxyRotation;
    ProxyRotation = GetActorRotation();
    ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

    if (FMath::Abs(ProxyYaw) > TurnThreshold)
    {
        if (ProxyYaw > TurnThreshold)
            TurningInPlace = ETurningInPlace::ETIP_Right;
        else if (ProxyYaw < -TurnThreshold)
            TurningInPlace = ETurningInPlace::ETIP_Left;
        else
            TurningInPlace = ETurningInPlace::ETIP_NotTurning;

        return;
    }

    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
    if (AO_Yaw > 90.f)
        TurningInPlace = ETurningInPlace::ETIP_Right;
    else if (AO_Yaw < -90.f)
        TurningInPlace = ETurningInPlace::ETIP_Left;

    if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
    {
        InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 8.f);
        AO_Yaw = InterpAO_Yaw;

        if (FMath::Abs(AO_Yaw) < 10.f)
        {
            TurningInPlace = ETurningInPlace::ETIP_NotTurning;
            StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        }
    }
}

void ABlasterCharacter::Jump()
{
    if (bDisableGameplay)
        return;

    if (bIsCrouched)
    {
        UnCrouch();
        Super::Jump();
    }
    else
        Super::Jump();
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
    UpdateHUDHealth();

    if (LastHealth > Health)
        PlayHitReactionMontage();
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
    UpdateHUDShield();

    if (LastShield > Shield)
        PlayHitReactionMontage();
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
    if (LastWeapon)
        LastWeapon->ShowPickupWidget(false);

    if (OverlappingWeapon)
        OverlappingWeapon->ShowPickupWidget(true);
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
    if (OverlappingWeapon)
        OverlappingWeapon->ShowPickupWidget(false);

    OverlappingWeapon = Weapon;

    if (IsLocallyControlled())
    {
        if (OverlappingWeapon)
            OverlappingWeapon->ShowPickupWidget(true);
    }
}

bool ABlasterCharacter::IsWeaponEquipped()
{
    return (CombatComponent && CombatComponent->GetEquippedWeapon());
}

bool ABlasterCharacter::IsAiming()
{
    return (CombatComponent && CombatComponent->IsAiming());
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
    if (CombatComponent == nullptr) return nullptr;

    return CombatComponent->GetEquippedWeapon();
}

FVector ABlasterCharacter::GetHitTarget()
{
    if (CombatComponent == nullptr)
        return FVector();

    return CombatComponent->GetHitTarget();
}

ECombatState ABlasterCharacter::GetCombatState()
{
    if (CombatComponent == nullptr) return ECombatState::ECS_MAX;

    return CombatComponent->GetCombatState();
}

 // Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Character/BlasterAnimInstance.h"
#include "Weapon/Weapon.h"
#include "BlasterComponents/CombatComponent.h"
#include "BlasterComponents/BuffComponent.h"
#include "BlasterComponents/LagCompensationComponent.h"
#include "Input/BlasterCharacterInputData.h"
#include "GameModes/BlasterGameMode.h"
#include "PlayerState/BlasterPlayerState.h"
#include "GameStates/BlasterGameState.h"
#include "Weapon/WeaponTypes.h"
#include "Enums/CombatState.h"

#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
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
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

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

    LagCompensationComponent = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

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

#pragma region Hitboxes for server side rewind

    head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
    head->SetupAttachment(GetMesh(), FName("head"));
    HitCollisionBoxes.Add(FName("head"), head);

    Pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("Pelvis"));
    Pelvis->SetupAttachment(GetMesh(), FName("Pelvis"));
    HitCollisionBoxes.Add(FName("Pelvis"), Pelvis);

    spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
    spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
    HitCollisionBoxes.Add(FName("spine_02"), spine_02);

    spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
    spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
    HitCollisionBoxes.Add(FName("spine_03"), spine_03);

    backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
    backpack->SetupAttachment(GetMesh(), FName("backpack"));
    HitCollisionBoxes.Add(FName("backpack"), backpack);

    blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
    blanket->SetupAttachment(GetMesh(), FName("backpack"));
    HitCollisionBoxes.Add(FName("blanket"), blanket);

    UpperArm_R = CreateDefaultSubobject<UBoxComponent>(TEXT("UpperArm_R"));
    UpperArm_R->SetupAttachment(GetMesh(), FName("UpperArm_R"));
    HitCollisionBoxes.Add(FName("UpperArm_R"), UpperArm_R);

    UpperArm_L = CreateDefaultSubobject<UBoxComponent>(TEXT("UpperArm_L"));
    UpperArm_L->SetupAttachment(GetMesh(), FName("UpperArm_L"));
    HitCollisionBoxes.Add(FName("UpperArm_L"), UpperArm_L);

    lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
    lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
    HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

    lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
    lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
    HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

    Hand_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Hand_R"));
    Hand_R->SetupAttachment(GetMesh(), FName("Hand_R"));
    HitCollisionBoxes.Add(FName("Hand_R"), Hand_R);

    Hand_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Hand_L"));
    Hand_L->SetupAttachment(GetMesh(), FName("Hand_L"));
    HitCollisionBoxes.Add(FName("Hand_L"), Hand_L);

    Thigh_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Thigh_R"));
    Thigh_R->SetupAttachment(GetMesh(), FName("Thigh_R"));
    HitCollisionBoxes.Add(FName("Thigh_R"), Thigh_R);

    Thigh_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Thigh_L"));
    Thigh_L->SetupAttachment(GetMesh(), FName("Thigh_L"));
    HitCollisionBoxes.Add(FName("Thigh_L"), Thigh_L);

    calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
    calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
    HitCollisionBoxes.Add(FName("calf_r"), calf_r);

    calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
    calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
    HitCollisionBoxes.Add(FName("calf_l"), calf_l);

    Foot_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Foot_R"));
    Foot_R->SetupAttachment(GetMesh(), FName("Foot_R"));
    HitCollisionBoxes.Add(FName("Foot_R"), Foot_R);

    Foot_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Foot_L"));
    Foot_L->SetupAttachment(GetMesh(), FName("Foot_L"));
    HitCollisionBoxes.Add(FName("Foot_L"), Foot_L);

    for (auto Box : HitCollisionBoxes)
    {
        if (Box.Value)
        {
            Box.Value->SetCollisionObjectType(ECC_HitBox);
            Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
            Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
            Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

#pragma endregion
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

    if (LagCompensationComponent)
    {
        LagCompensationComponent->SetOwningCharacter(this);

        if (Controller)
            LagCompensationComponent->SetOwningPlayerController(Cast<ABlasterPlayerController>(Controller));
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

            ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

            if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
                Multicast_GainedTheLead();
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
    PlayerInput->BindAction(InputActions->InputEquip, ETriggerEvent::Started, this, &ABlasterCharacter::Equip);
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

void ABlasterCharacter::Eliminated(bool bPlayerLeftGame)
{
    DropOrDestroyWeapons();

    Multicast_Eliminated(bPlayerLeftGame);
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
    if (CombatComponent)
    {
        if (CombatComponent->GetEquippedWeapon())
            DropOrDestroyWeapon(CombatComponent->GetEquippedWeapon());

        if (CombatComponent->GetSecondaryWeapon())
            DropOrDestroyWeapon(CombatComponent->GetSecondaryWeapon());
    }
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
    if (Weapon)
    {
        if (Weapon->GetDestroyOnElimination())
            Weapon->Destroy();
        else
            Weapon->Drop();
    }
}

void ABlasterCharacter::Multicast_Eliminated_Implementation(bool bPlayerLeftGame)
{
    if (BlasterPlayerController)
        BlasterPlayerController->SetHUDWeaponAmmo(0);

    bIsEliminated = true;
    bLeftGame = bPlayerLeftGame;

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

    if (CrownComponent)
        CrownComponent->DestroyComponent();

    GetWorldTimerManager().SetTimer(
        EliminatedTimer,
        this,
        &ABlasterCharacter::EliminatedTimerFinished,
        EliminatedDelay
    );
}

void ABlasterCharacter::EliminatedTimerFinished()
{
    ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();

    if (BlasterGameMode && !bLeftGame)
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

void ABlasterCharacter::Multicast_GainedTheLead_Implementation()
{
    if (CrownSystem == nullptr)
        return;

    if (CrownComponent == nullptr)
    {
        CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
            CrownSystem,
            GetCapsuleComponent(),
            FName(),
            GetActorLocation() + FVector(0.f, 0.f, 110.f),
            GetActorRotation(),
            EAttachLocation::KeepWorldPosition,
            false
        );
    }

    if (CrownComponent)
        CrownComponent->Activate();
}

void ABlasterCharacter::Multicast_LostTheLead_Implementation()
{
    if (CrownComponent)
        CrownComponent->DestroyComponent();
}

void ABlasterCharacter::Server_LeaveGame_Implementation()
{
    ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
    BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;

    if (BlasterGameMode && BlasterPlayerState)
        BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
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
        if (CombatComponent && GetCombatState() == ECombatState::ECS_Unoccupied)
            Server_Equip();

        bool bSwap = CombatComponent->ShouldSwapWeapon() &&
                     !HasAuthority() &&
                     CombatComponent->GetCombatState() == ECombatState::ECS_Unoccupied &&
                     OverlappingWeapon == nullptr;
        
        if (bSwap)
        {
            PlaySwapWeaponsMontage();
            CombatComponent->SetCombatState(ECombatState::ECS_SwappingWeapons);
            bFinishedSwappingWeapons = false;
        }
    }

    
}

void ABlasterCharacter::Server_Equip_Implementation()
{
    if (CombatComponent)
    {
        if (OverlappingWeapon)
            CombatComponent->EquipWeapon(OverlappingWeapon);
        else if (CombatComponent->ShouldSwapWeapon())
            CombatComponent->SwapWeapons();
    }
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

void ABlasterCharacter::PlaySwapWeaponsMontage()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    if (AnimInstance && SwapWeaponsMontage)
        AnimInstance->Montage_Play(SwapWeaponsMontage);
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

bool ABlasterCharacter::IsLocallyReloading()
{
    if (CombatComponent == nullptr)
        return false;

    return CombatComponent->GetIsLocallyReloading();
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

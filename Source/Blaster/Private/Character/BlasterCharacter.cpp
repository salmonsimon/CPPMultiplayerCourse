// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Input/BlasterCharacterInputData.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimMontage.h"
#include "Character/BlasterAnimInstance.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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
    CombatComponent->SetOwningCharacter(this);

    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

    GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

    TurningInPlace = ETurningInPlace::ETIP_NotTurning;

    NetUpdateFrequency = 66.f;
    MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
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

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABlasterCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AimOffset(DeltaTime);
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
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
    if (CombatComponent)
    {
        if (HasAuthority())
            CombatComponent->EquipWeapon(OverlappingWeapon);
        else
            ServerEquip();
    }
}

void ABlasterCharacter::ServerEquip_Implementation()
{
    CombatComponent->EquipWeapon(OverlappingWeapon);
}

void ABlasterCharacter::CrouchButtonPressed(const FInputActionValue& Value)
{
    if (GetCharacterMovement()->IsCrouching())
        UnCrouch();
    else
        Crouch();
}

void ABlasterCharacter::AimButtonPressed(const FInputActionValue& Value)
{
    if (CombatComponent)
        CombatComponent->SetIsAiming(true);
}

void ABlasterCharacter::AimButtonReleased(const FInputActionValue& Value)
{
    if (CombatComponent)
        CombatComponent->SetIsAiming(false);
}

void ABlasterCharacter::FireButtonPressed(const FInputActionValue& Value)
{
    if (CombatComponent)
        CombatComponent->FireButtonPressed(true);
}

void ABlasterCharacter::FireButtonReleased(const FInputActionValue& Value)
{
    if (CombatComponent)
        CombatComponent->FireButtonPressed(false);
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
    if (CombatComponent && CombatComponent->GetEquippedWeapon() == nullptr) return;

    FVector Velocity = GetVelocity();
    Velocity.Z = 0;

    float Speed = Velocity.Length();

    bool bIsInAir = GetCharacterMovement()->IsFalling();

    if (Speed == 0.f && !bIsInAir)
    {
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
        StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

        AO_Yaw = 0.f;
        bUseControllerRotationYaw = true;

        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    }

    AO_Pitch = GetBaseAimRotation().Pitch;
    if (AO_Pitch > 90.f && !IsLocallyControlled())
    {
        FVector2D InRange(270.f, 360.f);
        FVector2D OutRange(-90.f, 0.f);

        AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
    }
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
    if (bIsCrouched)
    {
        UnCrouch();
        Super::Jump();
    }
    else
        Super::Jump();
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
    PlayerInput->BindAction(InputActions->InputAim, ETriggerEvent::Triggered, this, &ABlasterCharacter::AimButtonPressed);
    PlayerInput->BindAction(InputActions->InputAim, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased);
    PlayerInput->BindAction(InputActions->InputFire, ETriggerEvent::Triggered, this, &ABlasterCharacter::FireButtonPressed);
    PlayerInput->BindAction(InputActions->InputFire, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased);
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



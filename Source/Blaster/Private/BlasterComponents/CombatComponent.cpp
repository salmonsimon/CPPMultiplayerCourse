// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"

#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h" 

#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);

	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	if (EquippedWeapon)
		EquippedWeapon->Drop();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->ShowPickupWidget(false);
	EquippedWeapon->SetHUDAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
		Controller->SetHUDCarriedAmmo(CarriedAmmo);

	if (EquippedWeapon->GetEquipSound())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->GetEquipSound(),
			Character->GetActorLocation()
		);
	}

	if (EquippedWeapon->IsEmpty())
		Reload();

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());

		if (EquippedWeapon->GetEquipSound())
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->GetEquipSound(),
				Character->GetActorLocation()
			);
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed && EquippedWeapon)
		Fire();
}

void UCombatComponent::Fire()
{
	if (!CanFire())
	{
		if (ShouldReloadInsteadOfFiring())
			Reload();

		return;
	}

	bCanFire = false;

	Server_Fire(HitTarget);

	if (EquippedWeapon)
		CrosshairShootingFactor = .75f;

	StartFireTimer();
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->GetFireDelay()
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;

	bCanFire = true;

	if (bFireButtonPressed && EquippedWeapon->GetIsAutomaticWeapon())
		Fire();

	if (bFireButtonPressed && EquippedWeapon->IsEmpty())
		Reload();
}

void UCombatComponent::Server_Fire_Implementation(const FVector_NetQuantize& TraceHitResult)
{
	Multicast_Fire(TraceHitResult);
}

void UCombatComponent::Multicast_Fire_Implementation(const FVector_NetQuantize& TraceHitResult)
{
	if (EquippedWeapon == nullptr) return;

	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitResult);
	}
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
		Server_Reload();
}

void UCombatComponent::Server_Reload_Implementation()
{
	CombatState = ECombatState::ECS_Reloading;

	HandleReload();
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;

	int32 MagazineRoom = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetCurrentAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 MaxAmountToReload = FMath::Min(MagazineRoom, CarriedAmmo);

		return FMath::Clamp(MagazineRoom, 0, MaxAmountToReload);
	}

	return 0;
}

void UCombatComponent::FinishedReloading()
{
	if (Character == nullptr) return;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}

	if (bFireButtonPressed)
		Fire();
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	int32 ReloadAmount = AmountToReload();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
		Controller->SetHUDCarriedAmmo(CarriedAmmo);

	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::SetIsAiming(bool IsAiming)
{
	bIsAiming = IsAiming;
	ServerSetIsAiming(IsAiming);

	if (Character)
		Character->GetCharacterMovement()->MaxWalkSpeed = IsAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::ServerSetIsAiming_Implementation(bool IsAiming)
{
	bIsAiming = IsAiming;

	if (Character)
		Character->GetCharacterMovement()->MaxWalkSpeed = IsAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bIsAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr)
		return;

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;

		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
			}
			else
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}

			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);

			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			else
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);

			if (bIsAiming)
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, .58f, DeltaTime, 30.f);
			else
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrosshairSpread =
				.5f +
				CrosshairVelocityFactor +
				CrosshairInAirFactor +
				CrosshairShootingFactor -
				CrosshairAimFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
		GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector LineTraceStart = CrosshairWorldPosition;

		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - LineTraceStart).Size();

			LineTraceStart += CrosshairWorldDirection * (DistanceToCharacter + 80.f);
		}

		FVector LineTraceEnd = LineTraceStart + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			LineTraceStart,
			LineTraceEnd,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit)
			TraceHitResult.ImpactPoint = LineTraceEnd;

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
			HUDPackage.CrosshairColor = FLinearColor::Red;
		else
			HUDPackage.CrosshairColor = FLinearColor::White;
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr || EquippedWeapon->IsEmpty() || !bCanFire || CombatState == ECombatState::ECS_Reloading)
		return false;

	return true;
}

bool UCombatComponent::ShouldReloadInsteadOfFiring()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && bCanFire && CombatState != ECombatState::ECS_Reloading && CarriedAmmo > 0)
		return true;

	return false;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
		case ECombatState::ECS_Reloading:

			HandleReload();

			break;

		case ECombatState::ECS_Unoccupied:

			if (bFireButtonPressed)
				Fire();

			break;
	}
}

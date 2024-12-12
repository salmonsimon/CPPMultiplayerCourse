// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "HUD/BlasterHUD.h"
#include "Weapon/WeaponTypes.h"
#include "Weapon/Weapon.h"
#include "Enums/CombatState.h"

#include "CombatComponent.generated.h"

class AWeapon;
class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterHUD;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* WeaponToEquip);
	void SwapWeapons();

	void FireButtonPressed(bool bPressed);

	void Fire();
	void LocalFire(const FVector_NetQuantize& TraceHitResult);

	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishedReloading();

	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector_NetQuantize& TraceHitResult);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Fire(const FVector_NetQuantize& TraceHitResult);

	UFUNCTION(Server, Reliable)
	void Server_Reload();

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);


protected:

	UFUNCTION(Server, Reliable)
	void ServerSetIsAiming(bool IsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

private:	

	void InterpFOV(float DeltaTime);

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();
	bool ShouldReloadInsteadOfFiring();

	void InitializeCarriedAmmo();

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UFUNCTION()
	void OnRep_CombatState();

	void HandleReload();
	int32 AmountToReload();
	void UpdateAmmoValues();

	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);

	ABlasterCharacter* Character;

	UPROPERTY()
	ABlasterPlayerController* Controller;

	UPROPERTY()
	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(Replicated)
	bool bIsAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 750.f;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.f;

	bool bFireButtonPressed;

	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FHUDPackage HUDPackage;

	FVector HitTarget;

	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	bool bCanFire = true;
	FTimerHandle FireTimer;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 45;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 StartingGranadeLauncherAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 300;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

public:
	FORCEINLINE void SetOwningCharacter(ABlasterCharacter* OwningCharacter) { Character = OwningCharacter; }
	FORCEINLINE AWeapon* GetEquippedWeapon() { return EquippedWeapon; }
	FORCEINLINE AWeapon* GetSecondaryWeapon() { return SecondaryWeapon; }
	FORCEINLINE FVector GetHitTarget() { return HitTarget; }
	FORCEINLINE bool IsAiming() { return bIsAiming; }
	FORCEINLINE ECombatState GetCombatState() { return CombatState; }
	FORCEINLINE int32 GetCarriedAmmo() { return CarriedAmmo; }
	FORCEINLINE int32 GetWeaponAmmo() { return EquippedWeapon->GetCurrentAmmo(); }

	void SetIsAiming(bool IsAiming);
	bool ShouldSwapWeapon();
};

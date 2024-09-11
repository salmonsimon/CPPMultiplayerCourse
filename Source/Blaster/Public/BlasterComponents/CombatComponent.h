// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"

#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

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

	void FireButtonPressed(bool bPressed);

	void Fire();

	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector_NetQuantize& TraceHitResult);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Fire(const FVector_NetQuantize& TraceHitResult);


protected:

	UFUNCTION(Server, Reliable)
	void ServerSetIsAiming(bool IsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

private:	

	void InterpFOV(float DeltaTime);

	void StartFireTimer();
	void FireTimerFinished();

	ABlasterCharacter* Character;
	ABlasterPlayerController* Controller;
	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

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

public:
	FORCEINLINE void SetOwningCharacter(ABlasterCharacter* OwningCharacter) { Character = OwningCharacter; }
	FORCEINLINE AWeapon* GetEquippedWeapon() { return EquippedWeapon; }
	FORCEINLINE FVector GetHitTarget() { return HitTarget; }
	FORCEINLINE bool IsAiming() { return bIsAiming; }

	void SetIsAiming(bool IsAiming);
};

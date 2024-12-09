// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Pickups/Pickup.h"
#include "Weapon/WeaponTypes.h"

#include "AmmoPickup.generated.h"



UCLASS()
class BLASTER_API AAmmoPickup : public APickup
{
	GENERATED_BODY()

protected:

	virtual void OnPickupSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	) override;

private:

	UPROPERTY(EditAnywhere, Category = "Pickup|Ammo Pickup")
	int32 AmmoAmount = 30;
	
	UPROPERTY(EditAnywhere, Category = "Pickup|Ammo Pickup")
	EWeaponType WeaponType;

};

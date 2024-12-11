// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShieldPickup : public APickup
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

	UPROPERTY(EditAnywhere, Category = "Pickup|ShieldBuff")
	float ShieldReplenishAmount = 100.f;

	UPROPERTY(EditAnywhere, Category = "Pickup|ShieldBuff")
	float ShieldReplenishTime = 5.f;

};

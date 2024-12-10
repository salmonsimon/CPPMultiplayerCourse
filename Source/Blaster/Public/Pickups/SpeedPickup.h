// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"

#include "SpeedPickup.generated.h"

UCLASS()
class BLASTER_API ASpeedPickup : public APickup
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
	);

private:

	UPROPERTY(EditAnywhere)
	float BaseSpeedBuff = 1000.f;

	UPROPERTY(EditAnywhere)
	float CrouchSpeedBuff = 850.f;

	UPROPERTY(EditAnywhere)
	float SpeedBuffTime = 15.f;
};

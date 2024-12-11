// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"

#include "HealthPickup.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class BLASTER_API AHealthPickup : public APickup
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

	UPROPERTY(EditAnywhere, Category = "Pickup|HealBuff")
	float HealAmount = 100.f;

	UPROPERTY(EditAnywhere, Category = "Pickup|HealBuff")
	float HealingTime = 5.f;	
	
};

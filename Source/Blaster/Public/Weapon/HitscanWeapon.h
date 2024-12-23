// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "HitscanWeapon.generated.h"

class UParticleSystem;
class USoundCue;

UCLASS()
class BLASTER_API AHitscanWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:

	virtual void Fire(const FVector& HitTarget) override;

protected:

	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult & OutHit);

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	USoundCue* HitSound;

private:

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", Meta  = (EditCondition = "FireAnimation==nullptr", EditConditionHides))
	UParticleSystem* MuzzleFlashParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", Meta = (EditCondition = "FireAnimation==nullptr", EditConditionHides))
	USoundCue* FireSound;
	
};

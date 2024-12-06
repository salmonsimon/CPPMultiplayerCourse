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

	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult & OutHit);

	UPROPERTY(EditAnywhere, Category = "Weapon|Combat")
	float Damage = 20.f;

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

	UPROPERTY(EditAnywhere, Category = "Weapon|Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere, Category = "Weapon|Weapon Scatter", Meta = (EditCondition = "bUseScatter"))
	float DistanceToScatterSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Weapon Scatter", Meta = (EditCondition = "bUseScatter"))
	float ScatterSphereRadius = 75.f;

	
};

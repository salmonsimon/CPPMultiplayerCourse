// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"

#include "ProjectileRocket.generated.h"

class USoundCue;
class UAudioComponent;
class USoundAttenuation;

UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();

protected:

	virtual void BeginPlay() override;
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void Destroyed() override;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoopSFX;

	UPROPERTY()
	UAudioComponent* ProjectileLoopSFXComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* ProjectileLoopSFXAttenuation;

private:

};

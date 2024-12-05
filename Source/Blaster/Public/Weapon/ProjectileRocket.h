// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"

#include "ProjectileRocket.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
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
	void DestroyTimerFinished();
	virtual void Destroyed() override;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;

	UPROPERTY()
	UNiagaraComponent* TrailNiagaraComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoopSFX;

	UPROPERTY()
	UAudioComponent* ProjectileLoopSFXComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* ProjectileLoopSFXAttenuation;

private:

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = true), Category = Damage)
	float MinimumDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = true), Category = Damage)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = true), Category = Damage)
	float DamageOuterRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = true), Category = Damage)
	float DamageFalloff = 1.f;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
};

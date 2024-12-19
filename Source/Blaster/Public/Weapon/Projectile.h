// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Projectile.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class USoundCue;


UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	void ApplyExplosionDamage();

	void StartDestroyTimer();
	void DestroyTimerFinished();

	virtual void SpawnImpactEffects();
	virtual void SpawnTrailSystem();

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;

	float Damage = 20.f;

	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere, Category = "Projectile|Effects")
	UNiagaraSystem* TrailSystem;

	UPROPERTY()
	UNiagaraComponent* TrailNiagaraComponent;

	UPROPERTY(EditAnywhere, Category = "Projectile|Configuration")
	bool bHasExplosiveProjectile = false;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Configuration", Meta = (EditCondition = "bHasExplosiveProjectile", EditConditionHides))
	float MinimumExplosionDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Configuration", Meta = (EditCondition = "bHasExplosiveProjectile", EditConditionHides))
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Configuration", Meta = (EditCondition = "bHasExplosiveProjectile", EditConditionHides))
	float DamageOuterRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Configuration", Meta = (EditCondition = "bHasExplosiveProjectile", EditConditionHides))
	float DamageFalloff = 1.f;

	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

private:

	UPROPERTY(EditAnywhere, Category = "Projectile|Effects")
	UParticleSystem* Tracer;

	UPROPERTY()
	UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere, Category = "Projectile|Effects")
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Projectile|Effects")
	USoundCue* ImpactSound;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere, Category = "Projectile|Configuration")
	float DestroyTime = 3.f;

public:

	FORCEINLINE float GetInitialSpeed() const { return InitialSpeed; }
	FORCEINLINE void SetUseServerSideRewind(bool Value) { bUseServerSideRewind = Value; }
	FORCEINLINE void SetTraceStart(FVector_NetQuantize NewTraceStart) { TraceStart = NewTraceStart; }
	FORCEINLINE void SetInitialVelocity(FVector_NetQuantize100 NewInitialVelocity) { InitialVelocity = NewInitialVelocity; }
	FORCEINLINE void SetDamage(float NewDamage) { Damage = NewDamage; }

};

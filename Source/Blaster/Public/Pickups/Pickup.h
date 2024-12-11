// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Pickup.generated.h"

class USphereComponent;
class USoundCue;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	

	APickup();

	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

	void EnableCustomDepth(bool bEnable);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPickupSphereOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:

	void BindOverlapTimerFinished();

	UPROPERTY(EditAnywhere, Category = "Pickup|Configuration")
	USphereComponent* PickupSphere;

	UPROPERTY(EditAnywhere, Category = "Pickup|Configuration")
	USoundCue* PickupSound;

	UPROPERTY(EditAnywhere, Category = "Pickup|Configuration")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditAnywhere, Category = "Pickup|Configuration")
	float BaseTurnRate = 45.f;

	UPROPERTY(EditAnywhere, Category = "Pickup|Configuration")
	UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(EditAnywhere, Category = "Pickup|Configuration")
	UNiagaraSystem* PickupEffect;

	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;

};

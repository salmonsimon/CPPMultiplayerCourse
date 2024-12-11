// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PickupSpawnPoint.generated.h"

class APickup;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	

	APickupSpawnPoint();

	virtual void Tick(float DeltaTime) override;


protected:

	virtual void BeginPlay() override;

	void SpawnPickup();
	void SpawnPickupTimerFinished();

	UFUNCTION()
	void StartSpawnTimer(AActor* DestroyedActor);

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<APickup>> PickupClasses;

	UPROPERTY()
	APickup* SpawnedPickup;

private:

	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin = 5.f;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax = 15.f;

};

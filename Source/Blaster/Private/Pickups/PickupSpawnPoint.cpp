// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpawnPoint.h"
#include "Pickups/Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	StartSpawnTimer((AActor*) nullptr);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::SpawnPickup()
{
	if (PickupClasses.IsEmpty())
		return;

	int32 SelectedClassIndex = FMath::RandRange(0, PickupClasses.Num() - 1);

	SpawnedPickup = GetWorld()->SpawnActor<APickup>(
		PickupClasses[SelectedClassIndex],
		GetActorTransform()
	);

	if (SpawnedPickup && HasAuthority())
		SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnTimer);
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority())
		SpawnPickup();
}

void APickupSpawnPoint::StartSpawnTimer(AActor* DestroyedActor)
{
	const float RandomSpawnTime = FMath::RandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);

	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this, 
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		RandomSpawnTime
	);
}

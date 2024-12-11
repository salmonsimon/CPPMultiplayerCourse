// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup.h"

#include "Weapon/WeaponTypes.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Sound/SoundCue.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(RootComponent);
	PickupSphere->SetSphereRadius(150.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupSphere->AddLocalOffset(FVector(0.f, 0.f, 85.f));

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(PickupSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetRelativeScale3D(FVector::OneVector * 3);

	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
	PickupMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			BindOverlapTimer,
			this,
			&APickup::BindOverlapTimerFinished,
			BindOverlapTime
		);
	}
		
}

void APickup::Destroyed()
{
	Super::Destroyed();

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PickupSound,
			GetActorLocation()
		);
	}

	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));
}

void APickup::OnPickupSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}

void APickup::EnableCustomDepth(bool bEnable)
{
	if (PickupMesh)
	{
		PickupMesh->SetRenderCustomDepth(bEnable);
	}
}

void APickup::BindOverlapTimerFinished()
{
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnPickupSphereOverlap);
}

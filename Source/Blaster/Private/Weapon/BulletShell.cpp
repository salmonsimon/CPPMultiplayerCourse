// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BulletShell.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h" 

ABulletShell::ABulletShell()
{
	PrimaryActorTick.bCanEverTick = false;

	BulletShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletShellMesh"));
	BulletShellMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	SetRootComponent(BulletShellMesh);

	BulletShellMesh->SetSimulatePhysics(true);
	BulletShellMesh->SetEnableGravity(true);
	BulletShellMesh->SetNotifyRigidBodyCollision(true);
}

void ABulletShell::BeginPlay()
{
	Super::BeginPlay();

	BulletShellMesh->OnComponentHit.AddDynamic(this, &ABulletShell::OnHit);

	BulletShellMesh->AddImpulse(GetActorForwardVector() * EjectionImpulse);
}

void ABulletShell::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (BulletShellSound)
		UGameplayStatics::PlaySoundAtLocation(this, BulletShellSound, GetActorLocation());

	SetLifeSpan(5.f);

	BulletShellMesh->SetNotifyRigidBodyCollision(false);
}



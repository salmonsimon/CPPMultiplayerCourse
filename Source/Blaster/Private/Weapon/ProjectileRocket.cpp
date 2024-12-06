// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "Kismet/GameplayStatics.h"

#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"

#include "NiagaraComponent.h"

#include "Sound/SoundCue.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);

	SpawnTrailSystem();

	if (ProjectileLoopSFX && ProjectileLoopSFXAttenuation)
	{
		ProjectileLoopSFXComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoopSFX,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.0f,
			1.0f,
			0.f,
			ProjectileLoopSFXAttenuation,
			nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ApplyExplosionDamage();

	SpawnImpactEffects();

	StartDestroyTimer();

	if (ProjectileMesh)
		ProjectileMesh->SetVisibility(false);

	if (CollisionBox)
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (TrailNiagaraComponent && TrailNiagaraComponent->GetSystemInstance())
		TrailNiagaraComponent->GetSystemInstance()->Deactivate();

	if (ProjectileLoopSFXComponent && ProjectileLoopSFXComponent->IsPlaying())
		ProjectileLoopSFXComponent->Stop();
}

void AProjectileRocket::Destroyed()
{

}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"

#include "Kismet/GameplayStatics.h"

#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Sound/SoundCue.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);

	if (TrailSystem)
	{
		TrailNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}

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
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff
			(
				this,
				Damage,
				MinimumDamage,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				DamageFalloff,
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				FiringController
			);
		}
	}

	SpawnImpactEffects();

	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this, 
		&AProjectileRocket::DestroyTimerFinished,
		DestroyTime
	);

	if (RocketMesh)
		RocketMesh->SetVisibility(false);

	if (CollisionBox)
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (TrailNiagaraComponent && TrailNiagaraComponent->GetSystemInstance())
		TrailNiagaraComponent->GetSystemInstance()->Deactivate();

	if (ProjectileLoopSFXComponent && ProjectileLoopSFXComponent->IsPlaying())
		ProjectileLoopSFXComponent->Stop();
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}

void AProjectileRocket::Destroyed()
{

}

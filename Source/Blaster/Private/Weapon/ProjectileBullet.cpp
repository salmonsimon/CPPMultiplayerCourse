// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"

#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "BlasterComponents/LagCompensationComponent.h"

#include "Kismet/GameplayStatics.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller);

		if (OwnerController)
		{
			if (OwnerController->HasAuthority() && !bUseServerSideRewind)
			{
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}

			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->IsLocallyControlled() && OwnerCharacter->GetLagCompensationComponent() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensationComponent()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->GetSingleTripTime()
				);
			}
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	/*
	FPredictProjectilePathParams PathParams;

	PathParams.ActorsToIgnore.Add(this);
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	PathParams.MaxSimTime = 4.f;
	PathParams.SimFrequency = 30.f;
	PathParams.StartLocation = GetActorLocation();
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;

	FPredictProjectilePathResult PathResult;

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
	*/
	
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName ChangedPropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif


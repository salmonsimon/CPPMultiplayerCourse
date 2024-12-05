// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitscanWeapon.h"

#include "Character/BlasterCharacter.h"
#include "Weapon/WeaponTypes.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

#include "DrawDebugHelpers.h"

void AHitscanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();


		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		
		ACharacter* HitCharacter = Cast<ACharacter>(FireHit.GetActor());

		if (HitCharacter && 
			HitCharacter != Cast<ACharacter>(OwnerPawn) &&
			HasAuthority() && 
			InstigatorController)
		{
			UGameplayStatics::ApplyDamage(
				HitCharacter,
				Damage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);
		}

		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}

		if (HitSound)
		{
			UGameplayStatics::SpawnSoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}

		if (MuzzleFlashParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlashParticles,
				SocketTransform
			);
		}

		if (FireSound)
		{
			UGameplayStatics::SpawnSoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

FVector AHitscanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector ScatterSphereCenter = TraceStart + ToTargetNormalized * DistanceToScatterSphere;

	FVector RandomScatterVector = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, ScatterSphereRadius);
	FVector EndLocation = ScatterSphereCenter + RandomScatterVector;
	FVector ToEndLocation = EndLocation - TraceStart;

	/*
	DrawDebugSphere(GetWorld(), ScatterSphereCenter, ScatterSphereRadius, 18, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLocation, 4.f, 18, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size(), FColor::Cyan, true);
	*/

	return FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size());
}

void AHitscanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult & OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector TraceEnd = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility
		);

		FVector BeamEnd = TraceEnd;

		if (OutHit.bBlockingHit)
			BeamEnd = OutHit.ImpactPoint;

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart
			);

			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}

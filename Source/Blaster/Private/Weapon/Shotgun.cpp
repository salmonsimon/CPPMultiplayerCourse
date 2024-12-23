// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"

#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "BlasterComponents/LagCompensationComponent.h"

#include "Engine/SkeletalMeshSocket.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "Particles/ParticleSystemComponent.h"

#include "Sound/SoundCue.h"

#include "DrawDebugHelpers.h"


void AShotgun::FireShotgun(const TArray<FVector_NetQuantize> HitTargets)
{
	AWeapon::Fire(FVector());

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector TraceStart = SocketTransform.GetLocation();

		TMap<ACharacter*, uint32> HitMap;

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(TraceStart, HitTarget, FireHit);

			ACharacter* HitCharacter = Cast<ACharacter>(FireHit.GetActor());

			if (HitCharacter && HitCharacter != Cast<ACharacter>(OwnerPawn))
			{
				if (HitMap.Contains(HitCharacter))
					HitMap[HitCharacter]++;
				else
					HitMap.Emplace(HitCharacter, 1);
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
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f,
					FMath::FRandRange(-.5f, .5f)
				);
			}
		}

		TArray<ABlasterCharacter*> HitCharacters;

		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();

				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						HitPair.Key,
						Damage * HitPair.Value,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}

				ABlasterCharacter* HitBlasterCharacter = Cast<ABlasterCharacter>(HitPair.Key);
				HitCharacters.Add(HitBlasterCharacter);
			}
		}

		if (!HasAuthority() && bUseServerSideRewind)
		{
			BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
			BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;

			if (BlasterOwnerController && BlasterOwnerCharacter && BlasterOwnerCharacter->GetLagCompensationComponent() && BlasterOwnerCharacter->IsLocallyControlled())
				BlasterOwnerCharacter->GetLagCompensationComponent()->ShotgunServerScoreRequest(
					HitCharacters, 
					TraceStart, 
					HitTargets, 
					BlasterOwnerController->GetServerTime() - BlasterOwnerController->GetSingleTripTime()
				);

		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutHitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr)
		return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector ScatterSphereCenter = TraceStart + ToTargetNormalized * DistanceToScatterSphere;

	for (uint32 i = 0; i < NumberOfpellets; i++)
	{
		const FVector RandomScatterVector = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, ScatterSphereRadius);
		const FVector EndLocation = ScatterSphereCenter + RandomScatterVector;

		FVector ToEndLocation = EndLocation - TraceStart;
		ToEndLocation = TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size();

		OutHitTargets.Add(ToEndLocation);
	}
}

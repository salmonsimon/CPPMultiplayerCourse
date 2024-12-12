// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"

#include "Character/BlasterCharacter.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector TraceStart = SocketTransform.GetLocation();

		TMap<ACharacter*, uint32> HitMap;

		for (uint32 i = 0; i < NumberOfpellets; i++)
		{
			FVector TraceEnd = TraceEndWithScatter(HitTarget);

			FHitResult FireHit;
			WeaponTraceHit(TraceStart, HitTarget, FireHit);

			ACharacter* HitCharacter = Cast<ACharacter>(FireHit.GetActor());

			if (HitCharacter &&
				HitCharacter != Cast<ACharacter>(OwnerPawn) &&
				HasAuthority() &&
				InstigatorController)
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

		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}

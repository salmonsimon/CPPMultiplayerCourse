// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Weapon/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	if (World && MuzzleFlashSocket && InstigatorPawn)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;

		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority())
			{
				if (InstigatorPawn->IsLocallyControlled()) // Server, Host - Use Replicated Projectile, No SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnedProjectile->SetUseServerSideRewind(false);
					SpawnedProjectile->SetDamage(Damage);
				}
				else // Server, Not Locally Controlled - Spawn Non-Replicated Projectile, SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnedProjectile->SetUseServerSideRewind(true);
				}
			}
			else
			{
				if (InstigatorPawn->IsLocallyControlled()) // Client, Locally Controlled - Spawn Non-Replicated Projectile, Use SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnedProjectile->SetUseServerSideRewind(true);

					SpawnedProjectile->SetTraceStart(SocketTransform.GetLocation());
					SpawnedProjectile->SetInitialVelocity(SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->GetInitialSpeed());

					SpawnedProjectile->SetDamage(Damage);
				}
				else // Client, Not Locally Controlled - Spawn Non-Replicated Projectile, No SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnedProjectile->SetUseServerSideRewind(false);
				}
			}
		}
		else
		{
			if (InstigatorPawn->HasAuthority()) // Spawn On Server Without SSR
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
				SpawnedProjectile->SetUseServerSideRewind(false);
				SpawnedProjectile->SetDamage(Damage);
			}
		}
	}
}
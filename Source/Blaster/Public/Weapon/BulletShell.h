// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletShell.generated.h"

UCLASS()
class BLASTER_API ABulletShell : public AActor
{
	GENERATED_BODY()

public:
	ABulletShell();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
		virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);


private:

	UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* BulletShellMesh;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
		float EjectionImpulse = 10.f;

	UPROPERTY(EditAnywhere)
		class USoundCue* BulletShellSound;
};

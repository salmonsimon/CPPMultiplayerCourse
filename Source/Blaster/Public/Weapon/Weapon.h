// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Weapon.generated.h"

class USkeletalMeshComponent;
class USphereComponent;
class UWidgetComponent;
class UAnimationAsset;
class ABulletShell;
class UTexture2D;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairLeft;

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairRight;

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairTop;

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairBottom;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPickupSphereOverlap
	(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnPickupSphereEndOverlap
	(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex
	);

private:

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	USphereComponent* PickupArea;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ABulletShell> BulletShellClass;

	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bIsAutomaticWeapon = true;

public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetPickupArea() const { return PickupArea; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE	bool GetIsAutomaticWeapon() const { return bIsAutomaticWeapon; }
};

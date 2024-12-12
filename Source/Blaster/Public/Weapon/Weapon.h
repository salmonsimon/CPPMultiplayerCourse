// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "WeaponTypes.h"

#include "Weapon.generated.h"

class USkeletalMeshComponent;
class USphereComponent;
class UWidgetComponent;
class UAnimationAsset;
class UTexture2D;
class USoundCue;

class ABlasterCharacter;
class ABlasterPlayerController;
class ABulletShell;


UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Hitscan UMETA(DisplayName = "Hitscan Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;

	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	void SetHUDAmmo();
	
	void Drop();

	void AddAmmo(int32 AmmoToAdd);

	void EnableCustomDepth(bool bEnable);

	FVector TraceEndWithScatter(const FVector& HitTarget);

	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshair")
	UTexture2D* CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshair")
	UTexture2D* CrosshairLeft;

	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshair")
	UTexture2D* CrosshairRight;

	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshair")
	UTexture2D* CrosshairTop;

	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshair")
	UTexture2D* CrosshairBottom;

protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped();

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

	UPROPERTY(EditAnywhere, Category = "Weapon|Main Configuration")
	UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon|Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere, Category = "Weapon|Weapon Scatter", Meta = (EditCondition = "bUseScatter"))
	float DistanceToScatterSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Weapon Scatter", Meta = (EditCondition = "bUseScatter"))
	float ScatterSphereRadius = 75.f;

private:

	UFUNCTION()
	void OnRep_WeaponState();

	UFUNCTION()
	void OnRep_Ammo();

	UFUNCTION()
	void SpendRound();

	UPROPERTY(VisibleAnywhere, Category = "Weapon|Main Configuration")
	USkeletalMeshComponent* WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon|Main Configuration")
	USphereComponent* PickupArea;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon|Main Configuration")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, Category = "Weapon|Main Configuration")
	UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon|Main Configuration")
	TSubclassOf<ABulletShell> BulletShellClass;

	UPROPERTY(EditAnywhere, Category = "Weapon|Main Configuration")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Main Configuration")
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Combat")
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Combat")
	bool bIsAutomaticWeapon = true;
	
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo, Category = "Weapon|Combat")
	int32 CurrentAmmo;

	UPROPERTY(EditAnywhere, Category = "Weapon|Combat")
	int32 MagazineCapacity;

	UPROPERTY()
	ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY()
	ABlasterPlayerController* BlasterOwnerController;

	UPROPERTY(EditAnywhere, Category = "Weapon|Main Configuration")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, Category = "Weapon|Main Configuration")
	EFireType FireType;

	UPROPERTY(EditAnywhere, Category = "Weapon|Main Configuration")
	USoundCue* EquipSound;

	bool bDestroyOnElimination = false;

public:
	void SetWeaponState(EWeaponState State);
	bool IsEmpty();

	FORCEINLINE USphereComponent* GetPickupArea() const { return PickupArea; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE	bool GetIsAutomaticWeapon() const { return bIsAutomaticWeapon; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE EFireType GetFireType() const { return FireType; }
	FORCEINLINE int32 GetCurrentAmmo() const { return CurrentAmmo; }
	FORCEINLINE int32 GetMagazineCapacity() const { return MagazineCapacity; }
	FORCEINLINE USoundCue* GetEquipSound() { return EquipSound; }
	FORCEINLINE bool GetDestroyOnElimination() { return bDestroyOnElimination; }
	FORCEINLINE void SetDestroyOnElimination(bool Value) { bDestroyOnElimination = Value; }
	FORCEINLINE bool GetUseScatter() const { return bUseScatter; }

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "Components/TimelineComponent.h"

#include "Enums/CombatState.h"
#include "Enums/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairInterface.h"


#include "BlasterCharacter.generated.h"

class ABlasterPlayerController;
class UBlasterCharacterInputData;
class AWeapon;
class UCombatComponent;
class UBuffComponent;
class ABlasterPlayerState;

class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class USoundCue;
class UParticleSystem;
class UParticleSystemComponent;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void PlayFireMontage(bool bIsAiming);
	void PlayReloadMontage();
	void PlayEliminatedMontage();
	
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitReaction();

	virtual void OnRep_ReplicatedMovement() override;

	void Eliminated();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Eliminated();

	virtual void Destroyed() override;

	void FireButtonPressed(const FInputActionValue& Value);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void UpdateHUDHealth();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Equip(const FInputActionValue& Value);
	void CrouchButtonPressed(const FInputActionValue& Value);
	void AimButtonPressed(const FInputActionValue& Value);
	void AimButtonReleased(const FInputActionValue& Value);
	void FireButtonReleased(const FInputActionValue& Value);
	void Reload(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void Server_Equip();

	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	virtual void Jump() override;

	void RotateInPlace(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	UInputMappingContext* InputMapping;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	UBlasterCharacterInputData* InputActions;

private:

	void PollInit();

	void HidePlayerIfTooClose();
	void PlayHitReactionMontage();

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_Health();

	void EliminatedTimerFinished();

	float CalculateSpeed();

	void StartDissolve();

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	UPROPERTY()
	ABlasterPlayerState* BlasterPlayerState;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraThreshold = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere)
	UBuffComponent* BuffComponent;

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	bool bRotateRootBone;
	float TurnThreshold = .5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactionMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* EliminatedMontage;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player State")
	float Health = 100.f;

	bool bIsEliminated = false;

	UPROPERTY()
	ABlasterPlayerController* BlasterPlayerController;

	FTimerHandle EliminatedTimer;

	UPROPERTY(EditDefaultsOnly)
	float EliminatedDelay = 3.f;

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UPROPERTY(VisibleAnywhere, Category = Elimination)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Elimination)
	UMaterialInstance* DissolveMaterialInstance;

	UPROPERTY(EditAnywhere)
	UParticleSystem* EliminationBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* EliminationBotComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* EliminationBotSound;


public:	
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FVector GetHitTarget();

	AWeapon* GetEquippedWeapon();

	ECombatState GetCombatState();

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool GetIsEliminated() const { return bIsEliminated; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE UCombatComponent* GetCombatComponent() { return CombatComponent; }
	FORCEINLINE bool GetDisableGameplay() { return bDisableGameplay; }

};

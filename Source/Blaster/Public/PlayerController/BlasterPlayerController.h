// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class ABlasterHUD;
class UCharacterOverlay;
class ABlasterGameMode;

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	virtual void Tick(float DeltaTime) override;

	virtual void ReceivedPlayer() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 CurrentAmmo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);


	void OnMatchStateSet(FName State);

	virtual float GetServerTime();

	FHighPingDelegate HighPingDelegate;

protected:

	virtual void BeginPlay() override;

	void PollInit();

	void SetHUDTime();

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float Starting);

	void CheckPing();
	void HighPingWarning();
	void StopHighPingWarning();

private:

	void SyncClientServerTimes();

	UFUNCTION()
	void OnRep_MatchState();

	void HandleMatchHasStarted();
	void HandleCooldown();

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY()
	ABlasterGameMode* BlasterGameMode;

	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	bool bInitializeHealth = false;
	bool bInitializeShield = false;
	bool bInitializeScore = false;
	bool bInitializeDefeats = false;
	bool bInitializeCarriedAmmo = false;
	bool bInitializeWeaponAmmo = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	int32 HUDScore;
	int32 HUDDefeats;
	int32 HUDCarriedAmmo;
	int32 HUDWeaponAmmo;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	float ClientServerDelta = 0;

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	FTimerHandle SyncTimesTimerHandle;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	FTimerHandle HighPingTimerHandle;
	FTimerHandle HighPingWarningStopTimerHandle;

	UPROPERTY(EditAnywhere)
	float HighPingWarningDuration = 5.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

	UPROPERTY()
	float SingleTripTime = 0.f;

public:

	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }

};

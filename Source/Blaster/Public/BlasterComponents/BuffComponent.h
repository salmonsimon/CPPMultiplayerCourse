// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "BuffComponent.generated.h"

class ABlasterCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealingTime);
	void BuffSpeed(float BaseSpeed, float CrouchSpeed, float BuffTime);

	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);

protected:
	virtual void BeginPlay() override;

	void HealRampUp(float DeltaTime);

private: 

	void ResetSpeeds();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpeedBuff(float BaseSpeed, float CrouchSpeed);

	UPROPERTY()
	ABlasterCharacter* Character;

	bool bHealing = false;
	float HealingRate = 0.0f;
	float AmountToHeal = 0.0f;

	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed = 0.f;
	float InitialCrouchSpeed = 0.f;

public:

	FORCEINLINE void SetOwningCharacter(ABlasterCharacter* OwningCharacter) { Character = OwningCharacter; }
		
};

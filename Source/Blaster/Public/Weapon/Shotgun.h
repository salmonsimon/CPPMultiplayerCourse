// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitscanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShotgun : public AHitscanWeapon
{
	GENERATED_BODY()

public: 

	virtual void Fire(const FVector& HitTarget) override;


private:

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfpellets = 10;
	
};

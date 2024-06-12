// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputAction.h"

#include "BlasterCharacterInputData.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterCharacterInputData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputMove;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputLook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputJump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputEquip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputCrouch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputAim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputFire;
};

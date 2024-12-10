// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/BuffComponent.h"

#include "Character/BlasterCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SetInitialJumpVelocity(float JumpVelocity)
{
	InitialJumpVelocity = JumpVelocity;
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;

	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || Character == nullptr || Character->GetIsEliminated())
	{
		return;
	}

	const float HealThisFrame = HealingRate * DeltaTime;

	Character->AddHealth(HealThisFrame);

	AmountToHeal -= HealThisFrame;

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0;
	}

	Character->UpdateHUDHealth();
}

void UBuffComponent::BuffSpeed(float BaseSpeed, float CrouchSpeed, float BuffTime)
{
	if (Character == nullptr)
		return;

	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&UBuffComponent::ResetSpeeds,
		BuffTime
	);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}

	Multicast_SpeedBuff(BaseSpeed, CrouchSpeed);
}

void UBuffComponent::Multicast_SpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
		return;

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::ResetSpeeds()
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
		return;

	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

	Multicast_SpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::BuffJump(float JumpSpeed, float BuffTime)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
		return;

	Character->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,
		this,
		&UBuffComponent::ResetJump,
		BuffTime
	);

	Character->GetCharacterMovement()->JumpZVelocity = JumpSpeed;

	Multicast_JumpBuff(JumpSpeed);
}

void UBuffComponent::Multicast_JumpBuff_Implementation(float JumpSpeed)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
		return;

	Character->GetCharacterMovement()->JumpZVelocity = JumpSpeed;
}

void UBuffComponent::ResetJump()
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
		return;

	Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
}
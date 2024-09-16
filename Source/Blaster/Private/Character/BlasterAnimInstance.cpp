// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacter == nullptr)
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());

	if (BlasterCharacter == nullptr)
		return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0;

	Speed = Velocity.Length();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling() ? true : false;
	bIsMoving = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouching = BlasterCharacter->bIsCrouched;
	bIsAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bIsEliminated = BlasterCharacter->GetIsEliminated();

	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator LocalDeltaMovementRotation = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaMovementRotation = FMath::RInterpTo(DeltaMovementRotation, LocalDeltaMovementRotation, DeltaTime, 6.f);
	YawOffset = DeltaMovementRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float TargetRotation = DeltaRotation.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, TargetRotation, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);

		FVector LeftHandOutPosition;
		FRotator LeftHandOutRotation;

		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, LeftHandOutPosition, LeftHandOutRotation);
		LeftHandTransform.SetLocation(LeftHandOutPosition);
		LeftHandTransform.SetRotation(FQuat(LeftHandOutRotation));

		IsLocal = BlasterCharacter->IsLocallyControlled();

		if (IsLocal)
		{
			RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);

			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotator = FMath::RInterpTo(RightHandRotator, LookAtRotation, DeltaTime, 30.f);
		}
	}
}

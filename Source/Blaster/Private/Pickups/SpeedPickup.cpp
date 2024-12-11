// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/SpeedPickup.h"

#include "Character/BlasterCharacter.h"
#include "BlasterComponents/BuffComponent.h"

void ASpeedPickup::OnPickupSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnPickupSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UBuffComponent* BuffComponent = BlasterCharacter->GetBuffComponent();

		if (BuffComponent)
		{
			BuffComponent->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
		}
	}

	Destroy();
}

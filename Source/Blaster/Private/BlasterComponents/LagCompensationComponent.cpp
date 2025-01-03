// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/LagCompensationComponent.h"

#include "Blaster/Blaster.h"
#include "Character/BlasterCharacter.h"
#include "BlasterComponents/CombatComponent.h"
#include "Weapon/Weapon.h"

#include "Components/BoxComponent.h"

#include "Kismet/GameplayStatics.h"

#include "DrawDebugHelpers.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && DamageCauser && Confirm.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageCauser->GetDamage(),
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (Character && HitCharacter && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Character->GetEquippedWeapon()->GetDamage(),
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character->GetCombatComponent()->GetEquippedWeapon() || Character == nullptr)
			continue;

		float HeadShotDamage;
		float BodyShotDamage;
		float TotalDamage;

		if (Confirm.HeadShots.Contains(HitCharacter))
			HeadShotDamage = Confirm.HeadShots[HitCharacter] * Character->GetCombatComponent()->GetEquippedWeapon()->GetDamage();

		if (Confirm.BodyShots.Contains(HitCharacter))
			BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetCombatComponent()->GetEquippedWeapon()->GetDamage();

		TotalDamage = HeadShotDamage + BodyShotDamage;

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(FFramePackage& PackageToCheck, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr)
		return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	MoveHitBoxes(HitCharacter, PackageToCheck);

	UBoxComponent* HeadHitBox = HitCharacter->GetHitCollisionBoxes()[FName("head")];
	HeadHitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadHitBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);

		if (ConfirmHitResult.bBlockingHit)
		{
			if (ConfirmHitResult.Component.IsValid())
			{
				UBoxComponent* DebugBox = Cast<UBoxComponent>(ConfirmHitResult.Component);
				if (DebugBox)
					DrawDebugBox(World, DebugBox->GetComponentLocation(), DebugBox->GetScaledBoxExtent(), FQuat(DebugBox->GetComponentRotation()), FColor::Red, false, 10.f);
			}

			MoveHitBoxes(HitCharacter, CurrentFrame, true);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{ true, true };
		}
		else
		{
			for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
				}

				World->LineTraceSingleByChannel(
					ConfirmHitResult,
					TraceStart,
					TraceEnd,
					ECC_HitBox
				);

				if (ConfirmHitResult.bBlockingHit)
				{
					if (ConfirmHitResult.Component.IsValid())
					{
						UBoxComponent* DebugBox = Cast<UBoxComponent>(ConfirmHitResult.Component);
						if (DebugBox)
							DrawDebugBox(World, DebugBox->GetComponentLocation(), DebugBox->GetScaledBoxExtent(), FQuat(DebugBox->GetComponentRotation()), FColor::Blue, false, 10.f);
					}

					MoveHitBoxes(HitCharacter, CurrentFrame, true);
					EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

					return FServerSideRewindResult{ true, false };
				}
			}
		}
	}

	MoveHitBoxes(HitCharacter, CurrentFrame, true);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	return FServerSideRewindResult{ false, false };
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(FFramePackage& PackageToCheck, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (HitCharacter == nullptr)
		return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	MoveHitBoxes(HitCharacter, PackageToCheck);

	UBoxComponent* HeadHitBox = HitCharacter->GetHitCollisionBoxes()[FName("head")];
	HeadHitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadHitBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FPredictProjectilePathParams PathParams;
	FPredictProjectilePathResult PathResult;

	PathParams.ActorsToIgnore.Add(GetOwner());

	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if (PathResult.HitResult.bBlockingHit)
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* DebugBox = Cast<UBoxComponent>(PathResult.HitResult.Component);
			if (DebugBox)
				DrawDebugBox(GetWorld(), DebugBox->GetComponentLocation(), DebugBox->GetScaledBoxExtent(), FQuat(DebugBox->GetComponentRotation()), FColor::Red, false, 10.f);
		}

		MoveHitBoxes(HitCharacter, CurrentFrame, true);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

		return FServerSideRewindResult{ true, true };
	}
	else
	{
		for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}

			UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
			if (PathResult.HitResult.bBlockingHit)
			{
				if (PathResult.HitResult.Component.IsValid())
				{
					UBoxComponent* DebugBox = Cast<UBoxComponent>(PathResult.HitResult.Component);
					if (DebugBox)
						DrawDebugBox(GetWorld(), DebugBox->GetComponentLocation(), DebugBox->GetScaledBoxExtent(), FQuat(DebugBox->GetComponentRotation()), FColor::Blue, false, 10.f);
				}

				MoveHitBoxes(HitCharacter, CurrentFrame, true);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

				return FServerSideRewindResult{ true, false };
			}
		}
	}

	MoveHitBoxes(HitCharacter, CurrentFrame, true);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& PackagesToCheck, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	for (auto& Frame : PackagesToCheck)
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();

	FShotgunServerSideRewindResult ShotgunResult;

	TArray<FFramePackage> CurrentFrames;
	for (auto Frame : PackagesToCheck)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;

		CacheBoxPositions(Frame.Character, CurrentFrame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		MoveHitBoxes(Frame.Character, Frame);

		CurrentFrames.Add(CurrentFrame);
	}

	for (auto& Frame : PackagesToCheck)
	{
		UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
	}

	UWorld* World = GetWorld();

	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHeadshotHitResult;

		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmHeadshotHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* HitBlasterCharacter = Cast<ABlasterCharacter>(ConfirmHeadshotHitResult.GetActor());
			if (HitBlasterCharacter)
			{
				if (ConfirmHeadshotHitResult.Component.IsValid())
				{
					UBoxComponent* DebugBox = Cast<UBoxComponent>(ConfirmHeadshotHitResult.Component);
					if (DebugBox)
						DrawDebugBox(World, DebugBox->GetComponentLocation(), DebugBox->GetScaledBoxExtent(), FQuat(DebugBox->GetComponentRotation()), FColor::Red, false, 10.f);
				}

				if (ShotgunResult.HeadShots.Contains(HitBlasterCharacter))
					ShotgunResult.HeadShots[HitBlasterCharacter]++;
				else
					ShotgunResult.HeadShots.Emplace(HitBlasterCharacter, 1);
			}
		}
	}

	for (auto& Frame : PackagesToCheck)
	{
		for (auto& HitBoxPair : Frame.Character->GetHitCollisionBoxes())
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
			}
		}

		UBoxComponent* HeadBox = Frame.Character->GetHitCollisionBoxes()[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmBodyHitResult;

		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmBodyHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* HitBlasterCharacter = Cast<ABlasterCharacter>(ConfirmBodyHitResult.GetActor());
			if (HitBlasterCharacter)
			{
				if (ConfirmBodyHitResult.Component.IsValid())
				{
					UBoxComponent* DebugBox = Cast<UBoxComponent>(ConfirmBodyHitResult.Component);
					if (DebugBox)
						DrawDebugBox(World, DebugBox->GetComponentLocation(), DebugBox->GetScaledBoxExtent(), FQuat(DebugBox->GetComponentRotation()), FColor::Blue, false, 10.f);
				}

				if (ShotgunResult.BodyShots.Contains(HitBlasterCharacter))
					ShotgunResult.BodyShots[HitBlasterCharacter]++;
				else
					ShotgunResult.BodyShots.Emplace(HitBlasterCharacter, 1);
			}
		}
	}

	for (auto& Frame : CurrentFrames)
	{
		MoveHitBoxes(Frame.Character, Frame, true);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotgunResult;
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f
		);
	}
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensationComponent() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetTail() == nullptr;

	if (bReturn)
		return FFramePackage();

	bool bShouldInterpolate = true;

	FFramePackage FrameToCheck;

	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;

	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;

	if (HitTime < OldestHistoryTime)
		return FFramePackage();

	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;

	while (HitTime < Older->GetValue().Time)
	{
		if (Older->GetNextNode() == nullptr)
			break;

		Older = Older->GetNextNode();

		if (HitTime > Older->GetValue().Time)
			Younger = Older;
	}

	if (Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);

	FrameToCheck.Character = HitCharacter;

	return FrameToCheck;
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerPair.Value;

		FBoxInformation InterpBoxInfo;

		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();

			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package, bool bResetHitBoxes)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);

			if (bResetHitBoxes)
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority())
		return;

	if (FrameHistory.Num() < 2)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
}


void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;

		for (auto& BoxPair : Character->GetHitCollisionBoxes())
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}


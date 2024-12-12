// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Weapon.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMeshSocket.h"

#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Weapon/BulletShell.h"

#include "Kismet/KismetMathLibrary.h"

#include "Net/UnrealNetwork.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	PickupArea = CreateDefaultSubobject<USphereComponent>(TEXT("PickupArea"));
	PickupArea->SetupAttachment(RootComponent);
	PickupArea->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, CurrentAmmo);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	PickupArea->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickupArea->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupArea->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnPickupSphereOverlap);
	PickupArea->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnPickupSphereEndOverlap); 

	if (PickupWidget)
		PickupWidget->SetVisibility(false);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::OnPickupSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnPickupSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:

		OnEquipped();
		break;

	case EWeaponState::EWS_EquippedSecondary:

		OnEquippedSecondary();
		break;

	case EWeaponState::EWS_Dropped:

		OnDropped();
		break;
	}
}


void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);

	PickupArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	EnableCustomDepth(false);
}

void AWeapon::OnEquippedSecondary()
{
	PickupArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	WeaponMesh->MarkRenderStateDirty();

	EnableCustomDepth(true);
}

void AWeapon::OnDropped()
{
	if (HasAuthority())
		PickupArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		
		if (BlasterOwnerController)
			BlasterOwnerController->SetHUDWeaponAmmo(CurrentAmmo);
	}
}

void AWeapon::SpendRound()
{
	CurrentAmmo = FMath::Clamp(CurrentAmmo - 1, 0, MagazineCapacity);

	SetHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
	SetHUDAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	CurrentAmmo = FMath::Clamp(CurrentAmmo + AmmoToAdd, 0, MagazineCapacity);

	SetHUDAmmo();
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr)
		return FVector();

	FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	FVector TraceStart = SocketTransform.GetLocation();

	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector ScatterSphereCenter = TraceStart + ToTargetNormalized * DistanceToScatterSphere;

	FVector RandomScatterVector = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, ScatterSphereRadius);
	FVector EndLocation = ScatterSphereCenter + RandomScatterVector;
	FVector ToEndLocation = EndLocation - TraceStart;

	/*
	DrawDebugSphere(GetWorld(), ScatterSphereCenter, ScatterSphereRadius, 18, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLocation, 4.f, 18, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size(), FColor::Cyan, true);
	*/

	return FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size());
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (Owner == nullptr)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(Owner) : BlasterOwnerCharacter;

		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
			SetHUDAmmo();
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
		PickupWidget->SetVisibility(bShowWidget);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
		WeaponMesh->PlayAnimation(FireAnimation, false);

	if (BulletShellClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));

		if (AmmoEjectSocket)
		{
			FTransform ScoketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			 UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ABulletShell>(
					BulletShellClass,
					ScoketTransform.GetLocation(),
					ScoketTransform.GetRotation().Rotator()
					);
			}
		}
	}

	if (HasAuthority())
		SpendRound();
}

void AWeapon::Drop()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);

	SetOwner(nullptr);

	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

bool AWeapon::IsEmpty()
{
	return CurrentAmmo <= 0;
}


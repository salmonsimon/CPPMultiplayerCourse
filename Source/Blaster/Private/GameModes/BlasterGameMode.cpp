// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "PlayerState/BlasterPlayerState.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
} 

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
			StartMatch();
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		//if (CountdownTime <= 0.f)
			//SetMatchState(MatchState::Cooldown);
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; Iterator++)
	{
		ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(*Iterator);

		if (PlayerController)
			PlayerController->OnMatchStateSet(MatchState);
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* EliminatedController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* EliminatedPlayerState = EliminatedController ? Cast<ABlasterPlayerState>(EliminatedController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != EliminatedPlayerState)
		AttackerPlayerState->AddToScore(1.f);

	if (EliminatedPlayerState)
		EliminatedPlayerState->AddToDefeats(1);

	if (EliminatedCharacter)
		EliminatedCharacter->Eliminated();
}

void ABlasterGameMode::RequestSpawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}

	if (EliminatedController)
	{
		TArray<AActor*> PlayerStartArray;

		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStartArray);

		int32 RandomPlayerStartSelection = FMath::RandRange(0, PlayerStartArray.Num() - 1);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStartArray[RandomPlayerStartSelection]);
	}
}

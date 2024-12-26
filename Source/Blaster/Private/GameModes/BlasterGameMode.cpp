// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "PlayerState/BlasterPlayerState.h"
#include "GameStates/BlasterGameState.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

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

		if (CountdownTime <= 0.f)
			SetMatchState(MatchState::Cooldown);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
			RestartGame();
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

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != EliminatedPlayerState && BlasterGameState)
	{
		TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;
		for (ABlasterPlayerState* LeadPlayer : BlasterGameState->TopScoringPlayers)
			PlayersCurrentlyInTheLead.Add(LeadPlayer);

		AttackerPlayerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);

		if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ABlasterCharacter* PlayerGainingTheLead = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());

			if (PlayerGainingTheLead)
				PlayerGainingTheLead->Multicast_GainedTheLead();
		}

		for (int i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (!BlasterGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ABlasterCharacter* PlayerLosingTheLead = Cast<ABlasterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());

				if (PlayerLosingTheLead)
					PlayerLosingTheLead->Multicast_LostTheLead();
			}

		}
	}

	if (EliminatedPlayerState)
		EliminatedPlayerState->AddToDefeats(1);

	if (EliminatedCharacter)
		EliminatedCharacter->Eliminated(false);
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

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr)
		return;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving))
		BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving);

	ABlasterCharacter* CharacterLeaving = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn());

	if (CharacterLeaving)
		CharacterLeaving->Eliminated(true);
}

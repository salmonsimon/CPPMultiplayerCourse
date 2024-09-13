// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "PlayerState/BlasterPlayerState.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* EliminatedController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* EliminatedPlayerState = EliminatedController ? Cast<ABlasterPlayerState>(EliminatedController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != EliminatedPlayerState)
		AttackerPlayerState->AddToScore(1.f);

	if (EliminatedPlayerState)
		EliminatedPlayerState->AddToDefeats();

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

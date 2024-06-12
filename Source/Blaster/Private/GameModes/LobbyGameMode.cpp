// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	if (NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true;

			FString BlasterMapPath = BlasterLevel.ToSoftObjectPath().ToString();

			int32 DotPosition;
			BlasterMapPath.FindLastChar('.', DotPosition);
			BlasterMapPath = BlasterMapPath.LeftChop(BlasterMapPath.Len() - DotPosition) + "?listen";

			World->ServerTravel(BlasterMapPath);
		}
	}
}
// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultithreadGameMode.h"
#include "MultithreadPlayerController.h"
#include "MultithreadCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMultithreadGameMode::AMultithreadGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AMultithreadPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// set default controller to our Blueprinted controller
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(
		TEXT("/Game/TopDown/Blueprints/BP_TopDownPlayerController"));
	if (PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
}

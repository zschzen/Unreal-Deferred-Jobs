// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultithreadGameMode.h"
#include "MultithreadHUD.h"
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

	// HUD hosts the UIBase screens (Rays view). Maps without a GameMode
	// Override (e.g. HUGE_MAP) fall back to this; BP_GM inherits it.
	// Prefers the HUD Blueprint; falls back to the C++ host on miss.
	static ConstructorHelpers::FClassFinder<AHUD> HUDBPClass(
		TEXT("/Game/Blueprints/UI/BP_MultithreadHUD"));
	if (HUDBPClass.Class != nullptr)
	{
		HUDClass = HUDBPClass.Class;
	}
	else
	{
		HUDClass = AMultithreadHUD::StaticClass();
	}
}

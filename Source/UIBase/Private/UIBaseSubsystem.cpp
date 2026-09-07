// Fill out your copyright notice in the Description page of Project Settings.

#include "UIBaseSubsystem.h"

#include "UIBaseController.h"
#include "UIBaseLog.h"
#include "UIBaseSettings.h"
#include "GameFramework/PlayerController.h"

void UUIBaseSubsystem::PlayerControllerChanged( APlayerController * NewPlayerController )
{
	// On level change, travel, possesion
	DestroyController();
	
	if ( !NewPlayerController ) return;
	
	UClass * ControllerClass = GetDefault< UUIBaseSettings >()->ControllerClass.LoadSynchronous();
	if ( !ControllerClass || ControllerClass->HasAnyClassFlags( CLASS_Abstract ) || !ControllerClass->IsChildOf( UUIBaseController::StaticClass() ) )
	{
		UE_LOGF( LogUIBase, Log,
			"UIBase: No valid `ControllerClass` configured in Project Settings > Game > UIBase"
			);
		return;
	}
	
	Controller = NewObject< UUIBaseController >( this, ControllerClass );
	Controller->Initialize( NewPlayerController );
}

void UUIBaseSubsystem::Deinitialize()
{
	DestroyController();
	
	Super::Deinitialize();
}

void UUIBaseSubsystem::DestroyController()
{
	if ( !Controller ) return;
	
	Controller->Deinitialize();
	Controller = nullptr;
}

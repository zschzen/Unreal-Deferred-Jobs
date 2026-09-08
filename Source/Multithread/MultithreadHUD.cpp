// Fill out your copyright notice in the Description page of Project Settings.


#include "MultithreadHUD.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Multithread.h"
#include "UI/RaysViewModel.h"
#include "UIBaseController.h"
#include "UIBaseSubsystem.h"

URaysViewModel *
AMultithreadHUD::GetRaysViewModel( TSubclassOf< URaysViewModel > ViewModelClass )
{
	const APlayerController * PC = GetOwningPlayerController();
	if ( !PC )
	{
		UE_LOG( LogMultithread, Warning, TEXT( "GetRaysViewModel: no OwningPlayerController." ) );
		return nullptr;
	}

	const ULocalPlayer * LP = PC->GetLocalPlayer();
	const UUIBaseSubsystem * Subsystem = LP ? LP->GetSubsystem< UUIBaseSubsystem >() : nullptr;
	UUIBaseController * Controller = Subsystem ? Subsystem->GetController() : nullptr;
	if ( !Controller )
	{
		UE_LOG( LogMultithread, Warning,
			TEXT( "GetRaysViewModel: UIBase Controller not ready yet." ) );
		return nullptr;
	}

	return Cast< URaysViewModel >( Controller->GetOrCreateViewModel( ViewModelClass ) );
}

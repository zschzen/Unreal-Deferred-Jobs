// Fill out your copyright notice in the Description page of Project Settings.


#include "UIBaseController.h"

#include "GameFramework/PlayerController.h"
#include "UIBaseLog.h"
#include "UIBaseViewModel.h"

void
UUIBaseController::Initialize( APlayerController * InOwningPlayerController )
{
	if ( bInitialized ) return;
	
	OwningPlayer = InOwningPlayerController;
	bInitialized = true;
	
	OnInitialize();
}

void
UUIBaseController::Deinitialize()
{
	if ( !bInitialized ) return;
	
	bInitialized = false;
	OnDeinitialize();
	
	for ( TPair< TSubclassOf< UUIBaseViewModel >, TObjectPtr< UUIBaseViewModel > >& Pair : ViewModels )
	{
		if ( Pair.Value ) Pair.Value->Deinitialize();
	}
	
	ViewModels.Empty();
	
	OwningPlayer.Reset();
}

UUIBaseViewModel *
UUIBaseController::GetOrCreateViewModel( TSubclassOf< UUIBaseViewModel > ViewModelClass )
{
	if ( !ViewModelClass || ViewModelClass->HasAnyClassFlags( CLASS_Abstract ) )
	{
		UE_LOGF( LogUIBase, Warning,
			"GetOrCreateViewModel: Invalid class '%ls'",
			*GetNameSafe( ViewModelClass )
			);
		return nullptr;
	}
	
	if ( TObjectPtr< UUIBaseViewModel > * Found = ViewModels.Find( ViewModelClass ) )
	{
		return * Found;
	}
	
	UUIBaseViewModel * NewViewModel = NewObject< UUIBaseViewModel >( this, ViewModelClass );
	NewViewModel->Initialize( this );
	ViewModels.Add( ViewModelClass, NewViewModel );
	return NewViewModel;
}

void
UUIBaseController::ReleaseViewModel( TSubclassOf< UUIBaseViewModel > ViewModelClass )
{
	TObjectPtr< UUIBaseViewModel > Removed = nullptr;
	if ( ViewModels.RemoveAndCopyValue( ViewModelClass, Removed ) && Removed )
	{
		Removed->Deinitialize();
	}
}

UWorld *
UUIBaseController::GetWorld() const
{
	if ( HasAnyFlags( RF_ClassDefaultObject ) ) return nullptr;
	
	if ( const APlayerController * PC = OwningPlayer.Get() ) return PC->GetWorld();
	if ( const UObject * Owner = GetOuter() ) return Owner->GetWorld();

	return nullptr;
}

void
UUIBaseController::BeginDestroy()
{
	Deinitialize();
	
	Super::BeginDestroy();
}

void
UUIBaseController::OnDeinitialize_Implementation()
{
	
}

void
UUIBaseController::OnInitialize_Implementation()
{
	
}

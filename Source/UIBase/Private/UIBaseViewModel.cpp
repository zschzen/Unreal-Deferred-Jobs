// Fill out your copyright notice in the Description page of Project Settings.


#include "UIBaseViewModel.h"

void
UUIBaseViewModel::Initialize( UObject * InContext )
{
	if ( bInitialized ) return;
	
	Context = InContext;
	bInitialized = true;
	
	OnInitialize();
}

void
UUIBaseViewModel::Deinitialize()
{
	if ( !bInitialized ) return;
	
	bInitialized = false;
	
	OnDeinitialize();
	Context.Reset();
}

UWorld *
UUIBaseViewModel::GetWorld() const
{
	// Prevents CDO
	if ( HasAnyFlags( RF_ClassDefaultObject ) ) return nullptr;
	
	if ( const UObject * Ctx = Context.Get() )
	{
		return Ctx->GetWorld();
	}
	
	if ( const UObject * Owner = GetOuter() )
	{
		return Owner->GetWorld();
	}
	
	return nullptr;
}

void
UUIBaseViewModel::OnDeinitialize_Implementation()
{
}

void
UUIBaseViewModel::OnInitialize_Implementation()
{
}

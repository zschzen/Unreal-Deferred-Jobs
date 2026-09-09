// Fill out your copyright notice in the Description page of Project Settings.


#include "RaysControl.h"

#include "RaysViewModel.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "RaysControl"

void
URaysControl::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Runs once per widget, unlike NativeConstruct, so the delegates are never
	// bound twice when the HUD is hidden and shown again
	if ( HideButton )
	{
		HideButton->OnClicked.AddDynamic( this, &URaysControl::HandleHideClicked );
	}

	if ( IncrementButton )
	{
		IncrementButton->OnClicked.AddDynamic( this, &URaysControl::HandleIncrementClicked );
	}

	if ( DecrementButton )
	{
		DecrementButton->OnClicked.AddDynamic( this, &URaysControl::HandleDecrementClicked );
	}
}

void
URaysControl::HandleHideClicked()
{
	if ( !BodyPanel )
	{
		return;
	}

	// The panel's own visibility is the state. A mirrored bool would only drift
	const bool bCollapsing = BodyPanel->GetVisibility() != ESlateVisibility::Collapsed;

	BodyPanel->SetVisibility( bCollapsing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible );

	if ( HideButtonText )
	{
		HideButtonText->SetText( bCollapsing
			? LOCTEXT( "ShowBody", "Show" )
			: LOCTEXT( "HideBody", "Hide" ) );
	}
}

void
URaysControl::HandleIncrementClicked()
{
	NudgeSlider( StepAmount );
}

void
URaysControl::HandleDecrementClicked()
{
	NudgeSlider( -StepAmount );
}

void
URaysControl::NudgeSlider( float Delta )
{
	URaysViewModel * const RaysViewModel = GetViewModelAs< URaysViewModel >();

	// A null ViewModel is a normal early frame, not an error: the Controller may
	// not exist yet. Same convention as UObserver::ResolveViewModel()
	if ( !RaysViewModel || !RaysSlider )
	{
		return;
	}

	// The range stays authored on the Slider in the Widget Blueprint, so the
	// clamp lives in exactly one place
	const float Nudged = FMath::Clamp(
		RaysViewModel->SliderValue + Delta,
		RaysSlider->GetMinValue(),
		RaysSlider->GetMaxValue() );

	// Through the ViewModel, never straight into the widget: RequestSliderValue
	// is what UObserver subscribes to
	RaysViewModel->RequestSliderValue( Nudged );
}

#undef LOCTEXT_NAMESPACE

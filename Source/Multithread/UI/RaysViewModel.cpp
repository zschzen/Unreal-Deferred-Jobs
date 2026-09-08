// Fill out your copyright notice in the Description page of Project Settings.


#include "RaysViewModel.h"

void
URaysViewModel::RequestSliderValue( float InValue )
{
	SetSliderValue( InValue );
	OnSliderValueRequested.Broadcast( InValue );
}

void
URaysViewModel::SetSliderValue( float InValue )
{
	// UE_MVVM_SET_PROPERTY_VALUE assigns and broadcasts only on change, which is
	// also what stops the Slider.Value binding from feeding itself
	if ( UE_MVVM_SET_PROPERTY_VALUE( SliderValue, InValue ) )
	{
		// The readout prints the percentage, so it went stale with the slider
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED( GetDisplayText );
	}
}

FText
URaysViewModel::GetDisplayText() const
{
	const int32 Percent = FMath::RoundToInt( SliderValue * 100.0f );
	return FText::FromString( FString::Printf(
		TEXT( "%d / %d rays (%d%%) - sweep %u: %d/%d gathered" ),
		RaysPerTimeSlice, TotalRays, Percent, SweepNumber, GatheredThisSweep, TotalRays ) );
}

void
URaysViewModel::SetBatchStats( int32 InRaysPerSlice, int32 InTotalRays, int32 InGathered, uint32 InSweep )
{
	// UObserver::TickComponent calls this every frame. Without this guard every
	// bound widget re-evaluates 60x/second for values that did not move
	if ( RaysPerTimeSlice == InRaysPerSlice
		&& TotalRays == InTotalRays
		&& GatheredThisSweep == InGathered
		&& SweepNumber == InSweep )
	{
		return;
	}

	RaysPerTimeSlice = InRaysPerSlice;
	TotalRays = InTotalRays;
	GatheredThisSweep = InGathered;
	SweepNumber = InSweep;

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED( GetDisplayText );
}

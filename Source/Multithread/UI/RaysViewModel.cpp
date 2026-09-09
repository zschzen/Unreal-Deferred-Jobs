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
	UE_MVVM_SET_PROPERTY_VALUE( SliderValue, InValue );
}

FText
URaysViewModel::GetDisplayText() const
{
	return FText::FromString( FString::Printf( TEXT( "%d / %d" ), GatheredThisSweep, TotalRays ) );
}

FText
URaysViewModel::GetRaysPerSliceText() const
{
	return FText::FromString( FString::FromInt( RaysPerTimeSlice ) );
}

float
URaysViewModel::GetBatchProgress() const
{
	return TotalRays > 0 ? static_cast<float>( GatheredThisSweep ) / static_cast<float>( TotalRays ) : 0.0f;
}

FText
URaysViewModel::GetRemainingText() const
{
	return FText::FromString( FString::Printf( TEXT( "remaining %d" ), FMath::Max( 0, TotalRays - GatheredThisSweep ) ) );
}

FText
URaysViewModel::GetBatchesText() const
{
	return FText::FromString( FString::Printf( TEXT( "# %u" ), SweepNumber ) );
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
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED( GetRaysPerSliceText );
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED( GetBatchProgress );
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED( GetRemainingText );
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED( GetBatchesText );
}

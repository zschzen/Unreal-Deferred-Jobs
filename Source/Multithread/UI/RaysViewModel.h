// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIBaseViewModel.h"

#include "RaysViewModel.generated.h"

UCLASS( BlueprintType )
class MULTITHREAD_API URaysViewModel : public UUIBaseViewModel
{
	GENERATED_BODY()

public:

	/** UI intent travelling outward. Gameplay subscribes; nothing else does */
	DECLARE_MULTICAST_DELEGATE_OneParam( FOnSliderValueRequested, float );
	FOnSliderValueRequested OnSliderValueRequested;

	/** Called by the Widget Blueprint from Slider.OnValueChanged */
	UFUNCTION( BlueprintCallable, Category = "Rays" )
	void RequestSliderValue( float InValue );

	/** Bound to Slider.Value. Seeded and corrected by gameplay */
	UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Rays" )
	float SliderValue = 0.01f;

	void SetSliderValue( float InValue );

	/** Bound to TextBlock.Text. Derived from the stats below; no backing storage */
	UFUNCTION( BlueprintPure, FieldNotify, Category = "Rays" )
	FText GetDisplayText() const;

	/** Pushed every frame by UObserver. Broadcasts only when a number actually moved */
	void SetBatchStats( int32 InRaysPerSlice, int32 InTotalRays, int32 InGathered, uint32 InSweep );

private:

	int32 RaysPerTimeSlice = 0;
	int32 TotalRays = 0;
	int32 GatheredThisSweep = 0;
	uint32 SweepNumber = 0;
};

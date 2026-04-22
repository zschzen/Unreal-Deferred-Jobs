// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RaysControl.generated.h"


// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSliderValueChanged, float,
                                            Value);

/**
 *
 */
UCLASS()
class MULTITHREAD_API URaysControl : public UUserWidget {
  GENERATED_BODY()

public:
  URaysControl(const FObjectInitializer &ObjectInitializer);

  virtual bool Initialize() override;

public:
  // Slider
  UPROPERTY(meta = (BindWidget))
  class USlider *Slider;

  UPROPERTY(meta = (BindWidgetOptional))
  class UTextBlock *DisplayText;

private:
  // On slider end value changed
  UFUNCTION()
  void OnSliderValueChanged(float Value);

public:
  // On slider value changed
  UPROPERTY(BlueprintAssignable)
  FOnSliderValueChanged OnSliderValueChangedDelegate{};

  void SetSliderValue(float Value) const;

  void SetRayBatchDisplay(int32 RaysPerTimeSlice, int32 TotalRays,
                          float SliderValue) const;
};

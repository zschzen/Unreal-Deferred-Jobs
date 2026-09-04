#include "RaysControl.h"

#include "Components/Slider.h"
#include "Components/TextBlock.h"

URaysControl::URaysControl(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

bool URaysControl::Initialize() {
  if (!Super::Initialize())
    return false;

  if (Slider)
    Slider->OnValueChanged.AddDynamic(this,
                                      &URaysControl::OnSliderValueChanged);

  return true;
}

void URaysControl::OnSliderValueChanged(float Value) {
  if (!OnSliderValueChangedDelegate.IsBound())
    return;
  OnSliderValueChangedDelegate.Broadcast(Value);
}

void URaysControl::SetSliderValue(float Value) const {
  if (!Slider)
    return;
  Value = FMath::Clamp(Value, Slider->GetMinValue(), Slider->GetMaxValue());
  Slider->SetValue(Value);
}

void URaysControl::SetRayBatchDisplay(int32 RaysPerTimeSlice, int32 TotalRays,
                                      float SliderValue, int32 GatheredThisSweep,
                                      uint32 SweepNumber) const {
  if (!DisplayText)
    return;

  const int32 Percent = FMath::RoundToInt(SliderValue * 100.0f);
  DisplayText->SetText(FText::FromString(FString::Printf(
      TEXT("%d / %d rays (%d%%) - sweep %u: %d/%d gathered"), RaysPerTimeSlice,
      TotalRays, Percent, SweepNumber, GatheredThisSweep, TotalRays)));
}

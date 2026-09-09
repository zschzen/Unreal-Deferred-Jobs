// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIBaseView.h"

#include "RaysControl.generated.h"

class UButton;
class USlider;
class UTextBlock;
class UWidget;

UCLASS( Abstract )
class MULTITHREAD_API URaysControl : public UUIBaseView
{
	GENERATED_BODY()

protected:

	//~ Begin UUserWidget interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget interface

	/** Collapses everything under the title row. View-local state; the ViewModel never sees it */
	UFUNCTION()
	void HandleHideClicked();

	UFUNCTION()
	void HandleIncrementClicked();

	UFUNCTION()
	void HandleDecrementClicked();

	/** Offsets the slider and routes the result through the ViewModel */
	void NudgeSlider( float Delta );

	/** Slider units per +/- press */
	UPROPERTY( EditDefaultsOnly, Category = "Rays" )
	float StepAmount = 0.01f;

	UPROPERTY( BlueprintReadOnly, Category = "Rays|Binding", meta = ( BindWidget ) )
	TObjectPtr< USlider > RaysSlider {};

	UPROPERTY( BlueprintReadOnly, Category = "Rays|Binding", meta = ( BindWidget ) )
	TObjectPtr< UTextBlock > RaysDisplayText {};

	UPROPERTY( BlueprintReadOnly, Category = "Rays|Binding", meta = ( BindWidget ) )
	TObjectPtr< UButton > HideButton {};

	UPROPERTY( BlueprintReadOnly, Category = "Rays|Binding", meta = ( BindWidget ) )
	TObjectPtr< UTextBlock > HideButtonText {};

	/** The collapsible container. Everything below the title row lives inside it */
	UPROPERTY( BlueprintReadOnly, Category = "Rays|Binding", meta = ( BindWidget ) )
	TObjectPtr< UWidget > BodyPanel {};

	UPROPERTY( BlueprintReadOnly, Category = "Rays|Binding", meta = ( BindWidget ) )
	TObjectPtr< UButton > DecrementButton {};

	UPROPERTY( BlueprintReadOnly, Category = "Rays|Binding", meta = ( BindWidget ) )
	TObjectPtr< UButton > IncrementButton {};

};

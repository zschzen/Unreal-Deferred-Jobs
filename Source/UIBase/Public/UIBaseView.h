// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "UIBaseView.generated.h"

class UUIBaseViewModel;

/**
 * 
 */
UCLASS(Blueprintable, Abstract)
class UIBASE_API UUIBaseView : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION( BlueprintCallable, Category = "UIBase" )
	void SetViewModel( UUIBaseViewModel * InViewModel );
	
	UFUNCTION( BlueprintPure, Category = "UIBase" )
	UE_NODISCARD_CTOR FORCEINLINE UUIBaseViewModel * GetViewModel() const { return ViewModel; }
	
protected:
	
	template < typename T >
	UE_NODISCARD_CTOR FORCEINLINE T * GetViewModelAs() const { return Cast< T >( ViewModel ); }
	
	UFUNCTION( BlueprintImplementableEvent, Category = "UIBase" )
	void OnViewModelSet();
	
	//~ Begin UUserWidget interface
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface
	
private:
	
	UPROPERTY( Transient )
	TObjectPtr< UUIBaseViewModel > ViewModel {};
};

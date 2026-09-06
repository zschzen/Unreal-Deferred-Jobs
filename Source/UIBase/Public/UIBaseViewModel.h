// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "UIBaseViewModel.generated.h"

/**
 * Base class for every ViewModel of UIBase
 */
UCLASS(Blueprintable, Abstract)
class UIBASE_API UUIBaseViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	
	void Initialize( UObject * InContext );
	
	void Deinitialize();
	
	UFUNCTION(BlueprintPure, Category = "UIBase")
	UE_NODISCARD_CTOR FORCEINLINE UObject * GetContext() const { return Context.Get(); }
	
	template < typename T >
	UE_NODISCARD_CTOR FORCEINLINE T * GetContextAs() const { return Cast< T >( Context.Get() ); }
	
	UFUNCTION( BlueprintPure, Category = "UIBase" )
	UE_NODISCARD_CTOR FORCEINLINE bool IsInitialized() const { return bInitialized; }
	
	//~ Begin UObject interface
	virtual UWorld * GetWorld() const override;
	//~ End UObject interface
	
protected:
	
	UFUNCTION( BlueprintNativeEvent, Category = "UIBase" )
	void OnInitialize();
	
	UFUNCTION( BlueprintNativeEvent, Category = "UIBase" )
	void OnDeinitialize();
	
private:
	
	/** Where this ViewModel reads from */
	TWeakObjectPtr< UObject > Context {};
	
	bool bInitialized = false;
};

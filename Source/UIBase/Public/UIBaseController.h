// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"

#include "UIBaseController.generated.h"

class APlayerController;
class UUIBaseViewModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FUIBaseScreenRequested, FGameplayTag, ScreenTag );

/**
 * 
 */
UCLASS( Blueprintable, Abstract )
class UIBASE_API UUIBaseController : public UObject
{
	GENERATED_BODY()
	
public:
	
	void Initialize( APlayerController * InOwningPlayerController );
	void Deinitialize();
	
	UFUNCTION( BlueprintCallable, Category = "UIBase", meta = ( DeterminesOutputType = "ViewModelClass" ) )
	UUIBaseViewModel * GetOrCreateViewModel( TSubclassOf< UUIBaseViewModel > ViewModelClass );
	
	UFUNCTION( BlueprintCallable, Category = "UIBase" )
	void ReleaseViewModel( TSubclassOf< UUIBaseViewModel > ViewModelClass );
	
	UFUNCTION( BlueprintCallable, Category = "UIBase" )
	void RequestScreen(const FGameplayTag ScreenTag ) const { OnScreenRequested.Broadcast( ScreenTag ); }
	
	UPROPERTY( BlueprintAssignable, Category = "UIBase" )
	FUIBaseScreenRequested OnScreenRequested {};
	
	//~ Begin UObject interface
	virtual UWorld * GetWorld() const override;
	virtual void BeginDestroy() override;
	//~ End UObject interface
	
protected:
	
	UFUNCTION( BlueprintNativeEvent, Category = "UIBase" )
	void OnInitialize();
	
	UFUNCTION( BlueprintNativeEvent, Category = "UIBase" )
	void OnDeinitialize();
	
private:
	
	TWeakObjectPtr< APlayerController > OwningPlayer {};
	
	UPROPERTY( Transient )
	TMap< TSubclassOf< UUIBaseViewModel >, TObjectPtr< UUIBaseViewModel > > ViewModels {};
	
	bool bInitialized = false;
};

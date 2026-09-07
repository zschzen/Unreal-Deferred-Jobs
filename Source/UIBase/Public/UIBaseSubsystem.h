// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UIBaseSubsystem.generated.h"

class UUIBaseController;

/**
 * 
 */
UCLASS()
class UIBASE_API UUIBaseSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
	
public:
	UFUNCTION( BlueprintPure, Category = "UIBase" )
	UE_NODISCARD_CTOR FORCEINLINE UUIBaseController * GetController() const { return Controller; }
	
	//~ Begin USubsystem interface
	virtual void Deinitialize() override;
	//~ End USubsystem interface
	
	//~ Begin ULocalPlayerSubsystem
	virtual void PlayerControllerChanged( APlayerController * NewPlayerController ) override;
	//~ End ULocalPlayerSubsystem
	
private:
	
	void DestroyController();
	
	UPROPERTY( Transient )
	TObjectPtr< UUIBaseController > Controller {};
};

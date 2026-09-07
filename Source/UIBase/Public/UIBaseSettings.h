// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UIBaseSettings.generated.h"

class UUIBaseController;

/**
 * 
 */
UCLASS( config = Game, DefaultConfig, meta = ( DisplayName = "UIBase" ) )
class UIBASE_API UUIBaseSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUIBaseSettings();
	
	UPROPERTY( EditAnywhere, Config, Category = "UIBase", meta = ( AllowAbstract = "false" ) )
	TSoftClassPtr< UUIBaseController > ControllerClass {};
	
};

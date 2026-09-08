// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "MultithreadHUD.generated.h"

class URaysViewModel;

UCLASS()
class MULTITHREAD_API AMultithreadHUD : public AHUD
{
	GENERATED_BODY()

public:

	UFUNCTION( BlueprintCallable, Category = "Rays" )
	URaysViewModel * GetRaysViewModel( TSubclassOf< URaysViewModel > ViewModelClass );
};

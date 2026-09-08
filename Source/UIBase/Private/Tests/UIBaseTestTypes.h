// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UIBaseController.h"
#include "UIBaseViewModel.h"

#include "UIBaseTestTypes.generated.h"

UCLASS()
class UUIBaseTestViewModel : public UUIBaseViewModel
{
	GENERATED_BODY()
};

UCLASS()
class UUIBaseTestOtherViewModel : public UUIBaseViewModel
{
	GENERATED_BODY()
};

UCLASS()
class UUIBaseTestController : public UUIBaseController
{
	GENERATED_BODY()
};

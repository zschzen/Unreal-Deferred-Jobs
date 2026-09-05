// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UIBaseLog.h"

DEFINE_LOG_CATEGORY(LogUIBase);

class FUIBaseModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogUIBase, Log, TEXT("UIBase module started"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogUIBase, Log, TEXT("UIBase module shutdown"));
	}
};

IMPLEMENT_MODULE(FUIBaseModule, UIBase);

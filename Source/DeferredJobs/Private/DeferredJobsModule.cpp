// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "DeferredJobsLog.h"

DEFINE_LOG_CATEGORY(LogDeferredJobs);

class FDeferredJobsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogDeferredJobs, Log, TEXT("DeferredJobs module started"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogDeferredJobs, Log, TEXT("DeferredJobs module shutdown"));
	}
};

IMPLEMENT_MODULE(FDeferredJobsModule, DeferredJobs);

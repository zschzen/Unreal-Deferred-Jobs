// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DeferredJobs : ModuleRules
{
	public DeferredJobs(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine"
		});
	}
}

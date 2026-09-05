// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Multithread : ModuleRules
{
	public Multithread(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.Default;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "NavigationSystem", "AIModule",
			"Niagara", "EnhancedInput", "RenderCore", "UMG", "Slate", "SlateCore", "DeferredJobs", "UIBase"
		});
	}
}
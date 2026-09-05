// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UIBase : ModuleRules
{
	public UIBase(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine"
		});
	}
}

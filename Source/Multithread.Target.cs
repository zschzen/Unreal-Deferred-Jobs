// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MultithreadTarget : TargetRules
{
	public MultithreadTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Multithread");
		ExtraModuleNames.Add("DeferredJobs");
		ExtraModuleNames.Add("UIBase");
	}
}

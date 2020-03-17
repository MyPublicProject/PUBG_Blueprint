// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MySpecialEffects : ModuleRules
{
	public MySpecialEffects(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UMG", "Slate", "SlateCore", "JSON", "HTTP", "NavigationSystem" });
        //注册页面我们要是使用JSON来向服务器发送数据。同时使用HTTP与服务器进行交互

        PrivateDependencyModuleNames.Add("OnlineSubsystemNull");
    }
}

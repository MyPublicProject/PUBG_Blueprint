// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class PUBG_BlueprintTarget : TargetRules
{
	public PUBG_BlueprintTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "PUBG_Blueprint" } );
	}
}

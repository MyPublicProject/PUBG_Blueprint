// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class PUBG_BlueprintEditorTarget : TargetRules
{
	public PUBG_BlueprintEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "PUBG_Blueprint" } );
	}
}

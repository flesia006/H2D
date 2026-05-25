// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class TP_ZDHybridTopDownBPEditorTarget : TargetRules
{
	public TP_ZDHybridTopDownBPEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "TP_ZDHybridTopDownBP" } );
	}
}

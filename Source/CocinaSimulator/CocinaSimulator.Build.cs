// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CocinaSimulator : ModuleRules
{
	public CocinaSimulator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"PhotonFusion",
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CocinaSimulator",
			"CocinaSimulator/Variant_Platforming",
			"CocinaSimulator/Variant_Platforming/Animation",
			"CocinaSimulator/Variant_Combat",
			"CocinaSimulator/Variant_Combat/AI",
			"CocinaSimulator/Variant_Combat/Animation",
			"CocinaSimulator/Variant_Combat/Gameplay",
			"CocinaSimulator/Variant_Combat/Interfaces",
			"CocinaSimulator/Variant_Combat/UI",
			"CocinaSimulator/Variant_SideScrolling",
			"CocinaSimulator/Variant_SideScrolling/AI",
			"CocinaSimulator/Variant_SideScrolling/Gameplay",
			"CocinaSimulator/Variant_SideScrolling/Interfaces",
			"CocinaSimulator/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

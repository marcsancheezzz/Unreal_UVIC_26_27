using UnrealBuildTool;

public class MotorsVideojocs_UVICEditor : ModuleRules
{
	public MotorsVideojocs_UVICEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler", "AssetRegistry" });
	}
}

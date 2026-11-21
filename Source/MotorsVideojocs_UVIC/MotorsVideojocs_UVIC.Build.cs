using UnrealBuildTool;

public class MotorsVideojocs_UVIC : ModuleRules
{
	public MotorsVideojocs_UVIC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG", "Slate", "SlateCore", "InputCore" });
	}
}

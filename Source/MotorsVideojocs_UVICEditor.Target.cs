using UnrealBuildTool;

public class MotorsVideojocs_UVICEditorTarget : TargetRules
{
	public MotorsVideojocs_UVICEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "MotorsVideojocs_UVIC", "MotorsVideojocs_UVICEditor" });
	}
}

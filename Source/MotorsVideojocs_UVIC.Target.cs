using UnrealBuildTool;

public class MotorsVideojocs_UVICTarget : TargetRules
{
	public MotorsVideojocs_UVICTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MotorsVideojocs_UVIC");
	}
}

using UnrealBuildTool;
public class NuxieLabEditorTarget : TargetRules {
  public NuxieLabEditorTarget(TargetInfo Target) : base(Target) {
    Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.V7;
    IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
    ExtraModuleNames.Add("NuxieLab");
  }
}

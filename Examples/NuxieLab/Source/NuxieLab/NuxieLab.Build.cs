using UnrealBuildTool;
using System.IO;
public class NuxieLab : ModuleRules {
  public NuxieLab(ReadOnlyTargetRules Target) : base(Target) {
    if (Target.Platform == UnrealTargetPlatform.Android && Target.Configuration == UnrealTargetConfiguration.Development)
      AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "LabDevelopment_APL.xml"));
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    PublicDependencyModuleNames.AddRange(new[] {"Core", "CoreUObject", "Engine", "Nuxie", "UMG", "Slate", "SlateCore", "Json"});
  }
}

using UnrealBuildTool;
public class NuxieEditor : ModuleRules {
  public NuxieEditor(ReadOnlyTargetRules Target) : base(Target) {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    PrivateDependencyModuleNames.AddRange(new[] {"Core", "CoreUObject", "Engine", "Nuxie", "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler"});
  }
}

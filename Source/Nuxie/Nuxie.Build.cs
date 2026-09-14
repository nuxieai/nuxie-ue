using System;
using System.IO;
using System.Security.Cryptography;
using EpicGames.Core;
using UnrealBuildTool;

public class Nuxie : ModuleRules
{
  public Nuxie(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    CppStandard = CppStandardVersion.Cpp20;

    PublicDependencyModuleNames.AddRange(new string[]
    {
      "Core",
      "CoreUObject",
      "DeveloperSettings",
      "Engine",
      "Projects",
      "Json"
    });

    PrivateDependencyModuleNames.AddRange(new string[]
    {
      "ApplicationCore",
      "Projects"
    });

    if (Target.Platform == UnrealTargetPlatform.Android)
    {
      PrivateDependencyModuleNames.Add("Launch");
      string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
      VerifyArtifacts(PluginPath, "Android");
      string AplPath = Path.Combine(PluginPath, "ThirdParty/Android/Nuxie_APL.xml");
      AdditionalPropertiesForReceipt.Add("AndroidPlugin", AplPath);
    }

    if (Target.Platform == UnrealTargetPlatform.IOS)
    {
      PrivateDependencyModuleNames.Add("Swift");
      string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
      VerifyArtifacts(PluginPath, "IOS");
      string FrameworkZip = Path.Combine(
        PluginPath,
        Target.Architecture == UnrealArch.IOSSimulator
          ? "ThirdParty/IOS/lib/simulator/NuxieUnrealBridge.embeddedframework.zip"
          : "ThirdParty/IOS/lib/ios/NuxieUnrealBridge.embeddedframework.zip"
      );
      if (!File.Exists(FrameworkZip))
      {
        throw new BuildException(
          "Missing Nuxie iOS bridge. Run ThirdParty/IOS/scripts/build-framework.sh."
        );
      }
      PublicAdditionalFrameworks.Add(
        new Framework("NuxieUnrealBridge", FrameworkZip, null, true)
      );
      PublicFrameworks.AddRange(new string[]
      {
        "CoreGraphics",
        "Foundation",
        "Metal",
        "QuartzCore",
        "Security",
        "StoreKit",
        "WebKit"
      });
      PublicWeakFrameworks.AddRange(new string[] { "AdSupport" });
    }
  }
  private static void VerifyArtifacts(string Root, string Platform)
  {
    string Receipt = Path.Combine(Root, "ThirdParty", Platform, "lib", "receipt.json");
    if (!File.Exists(Receipt)) throw new BuildException("Prepare Nuxie's pinned native artifacts before packaging. Missing " + Receipt);
    JsonObject Document = JsonObject.Read(new FileReference(Receipt));
    foreach (string Section in new[] {"inputs", "artifacts"})
    {
      JsonObject Entries = Document.GetObjectField(Section);
      foreach (string Entry in Entries.KeyNames)
      {
        string FilePath = Path.GetFullPath(Path.Combine(Root, Entry));
        if (!FilePath.StartsWith(Path.TrimEndingDirectorySeparator(Path.GetFullPath(Root)) + Path.DirectorySeparatorChar, StringComparison.Ordinal)) throw new BuildException("Invalid Nuxie artifact receipt path.");
        // Consumer archives omit native source; their pin manifest and every artifact remain mandatory.
        bool IncludesNativeSource = Directory.Exists(Path.Combine(Root, "ThirdParty", Platform, Platform == "IOS" ? "Sources" : "bridge/src/main"));
        if (!File.Exists(FilePath) && !IncludesNativeSource && Section == "inputs" && Entry != "NATIVE-PINS.json") continue;
        if (!File.Exists(FilePath)) throw new BuildException("Missing prepared Nuxie artifact: " + Entry);
        using var Stream = File.OpenRead(FilePath);
        string Actual = Convert.ToHexString(SHA256.HashData(Stream)).ToLowerInvariant();
        if (Actual != Entries.GetStringField(Entry)) throw new BuildException("Stale or modified Nuxie native input/artifact: " + Entry + ". Prepare native artifacts again.");
      }
    }
  }

}

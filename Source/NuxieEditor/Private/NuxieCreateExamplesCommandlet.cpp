#include "NuxieCreateExamplesCommandlet.h"
#include "NuxieAsyncActions.h"
#include "NuxieSettings.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"
#include "Modules/ModuleManager.h"
IMPLEMENT_MODULE(FDefaultModuleImpl, NuxieEditor)
UNuxieCreateExamplesCommandlet::UNuxieCreateExamplesCommandlet() { IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UNuxieCreateExamplesCommandlet::Main(const FString& Params) {
  const FString Asset = TEXT("/Game/BP_NuxieGettingStarted");
  // Asset generation is explicit and refuses to overwrite edited teaching graphs.
  if (FPackageName::DoesPackageExist(Asset)) { UE_LOG(LogTemp, Error, TEXT("Example assets already exist; preserving them.")); return 1; }
  auto* Package = CreatePackage(*Asset);
  auto* Blueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, TEXT("BP_NuxieGettingStarted"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
  auto* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
  if (!Graph) return 2;
  // Start with a single, explicit BeginPlay instead of the editor's disabled event placeholders.
  for (auto* Node : TArray<UEdGraphNode*>(Graph->Nodes)) Graph->RemoveNode(Node);
  auto* Begin = NewObject<UK2Node_Event>(Graph); Graph->AddNode(Begin, false, false); Begin->CreateNewGuid();
  Begin->EventReference.SetExternalMember(TEXT("ReceiveBeginPlay"), AActor::StaticClass()); Begin->bOverrideFunction = true; Begin->AllocateDefaultPins();
  auto* Options = NewObject<UK2Node_CallFunction>(Graph); Graph->AddNode(Options, false, false); Options->CreateNewGuid();
  Options->SetFromFunction(UNuxieSettings::StaticClass()->FindFunctionByName(TEXT("GetProjectOptions"))); Options->AllocateDefaultPins(); Options->NodePosY = 180;
  auto Async = [Graph](UClass* Class, FName Function, int32 X) {
    auto* Node = NewObject<UK2Node_AsyncAction>(Graph); Graph->AddNode(Node, false, false); Node->CreateNewGuid(); Node->InitializeProxyFromFunction(Class->FindFunctionByName(Function)); Node->AllocateDefaultPins(); Node->NodePosX = X; return Node;
  };
  auto* Configure = Async(UNuxieConfigureAsyncAction::StaticClass(), TEXT("Configure"), 360);
  auto* Identity = Async(UNuxieGetIdentityAsyncAction::StaticClass(), TEXT("GetIdentity"), 800);
  auto Print = [Graph](const FString& Text, int32 Y) {
    auto* Node = NewObject<UK2Node_CallFunction>(Graph); Graph->AddNode(Node, false, false); Node->CreateNewGuid(); Node->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString"))); Node->AllocateDefaultPins(); Node->NodePosX = 1240; Node->NodePosY = Y; Node->FindPinChecked(TEXT("InString"))->DefaultValue = Text; return Node;
  };
  auto* Success = Print(TEXT("Nuxie: coherent identity received. Inspect the typed Identity output in this graph."), 0);
  auto* Failure = Print(TEXT("Nuxie failed: inspect the typed Error output. Desktop commands return UnsupportedPlatform."), 350);
  const auto* Schema = GetDefault<UEdGraphSchema_K2>();
  bool bConnected = Schema->TryCreateConnection(Begin->FindPinChecked(UEdGraphSchema_K2::PN_Then), (Params.Contains(TEXT("BlueprintOnly")) ? Configure : Identity)->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
  bConnected &= Schema->TryCreateConnection(Options->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue), Configure->FindPinChecked(TEXT("Options")));
  bConnected &= Schema->TryCreateConnection(Configure->FindPinChecked(TEXT("Success")), Identity->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
  bConnected &= Schema->TryCreateConnection(Identity->FindPinChecked(TEXT("Success")), Success->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
  bConnected &= Schema->TryCreateConnection(Configure->FindPinChecked(TEXT("Failure")), Failure->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
  bConnected &= Schema->TryCreateConnection(Identity->FindPinChecked(TEXT("Failure")), Failure->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
  if (!bConnected) return 3;
  FCompilerResultsLog Results; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
  if (Results.NumErrors || Blueprint->Status == BS_Error) return 4;
  FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
  const FString Filename = FPackageName::LongPackageNameToFilename(Asset, FPackageName::GetAssetPackageExtension());
  IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
  if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs)) return 5;
  for (const TCHAR* Map : {TEXT("Lab"), TEXT("LabSecond")}) {
    auto* World = GEditor->NewMap();
    if (Params.Contains(TEXT("BlueprintOnly")) || FString(Map) == TEXT("LabSecond")) World->SpawnActor<AActor>(Blueprint->GeneratedClass);
    if (!FEditorFileUtils::SaveMap(World, FPaths::ProjectContentDir() / (FString(Map) + TEXT(".umap")))) return 6;
  }
  UE_LOG(LogTemp, Display, TEXT("Nuxie examples: compiled Blueprint and saved both maps.")); return 0;
}

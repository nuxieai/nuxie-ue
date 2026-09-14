#include "NuxieLab.h"
#include "NuxieSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SafeZone.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"
#include "Misc/Guid.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, NuxieLab, "NuxieLab");
namespace {
// Native translation of Nuxie's dark canvas, smoked surfaces and foreground tokens.
const FLinearColor Canvas(0.018f, 0.017f, 0.025f, 1);
const FLinearColor Surface(0.09f, 0.09f, 0.11f, 1);
const FLinearColor Hover(0.14f, 0.14f, 0.17f, 1);
const FLinearColor Primary(0.36f, 0.20f, 0.78f, 1);
const FLinearColor Foreground(0.94f, 0.94f, 0.97f, 1);
const TCHAR* SlotName = TEXT("NuxieLabPendingOperation");
}
void UNuxieLabBilling::BeginPurchase_Implementation(UNuxiePurchaseRequest* Request) {
  if (Purchase && Purchase->IsPending()) { Request->TryComplete(ENuxiePurchaseOutcome::Failed, TEXT("The Lab already has a checkout.")); return; }
  Purchase = Request;
  UE_LOG(LogTemp, Display, TEXT("NuxieLab external checkout retained. Use Cancel external checkout, or wait for the native deadline. Inspect Purchase.Product for the selected store offer."));
}
void UNuxieLabBilling::BeginRestore_Implementation(UNuxieRestoreRequest* Request) {
  if (Restore && Restore->IsPending()) { Request->TryComplete(ENuxieRestoreOutcome::Failed, TEXT("The Lab already has a restore.")); return; }
  Restore = Request;
  UE_LOG(LogTemp, Display, TEXT("NuxieLab external restore retained. Use Fail external restore, or wait for the native deadline."));
}
bool UNuxieLabBilling::CancelPurchase() { return Purchase && Purchase->TryComplete(ENuxiePurchaseOutcome::Cancelled, TEXT("Cancelled in the Lab's external billing harness.")); }
bool UNuxieLabBilling::FailRestore() { return Restore && Restore->TryComplete(ENuxieRestoreOutcome::Failed, TEXT("No external store provider is connected to this Lab harness.")); }
void UNuxieLabBilling::Deinitialize() { CancelPurchase(); FailRestore(); Purchase = nullptr; Restore = nullptr; Super::Deinitialize(); }
ANuxieLabGameMode::ANuxieLabGameMode() { PlayerControllerClass = ANuxieLabController::StaticClass(); DefaultPawnClass = nullptr; }
void ANuxieLabController::BeginPlay() {
  Super::BeginPlay(); bShowMouseCursor = true;
  auto* Widget = CreateWidget<UNuxieLabWidget>(this); Widget->AddToViewport(); SetInputMode(FInputModeUIOnly());
}
void UNuxieLabWidget::Button(const FString& Label, TFunction<void()> Action) {
  auto* Control = WidgetTree->ConstructWidget<UNuxieLabButton>(); Control->Action = [this, Action = MoveTemp(Action)]() { if (bOperationPending) { Log(TEXT("Wait for the pending operation to settle.")); return; } Action(); };
  Control->OnClicked.AddDynamic(Control, &UNuxieLabButton::Run);
  FButtonStyle Style = Control->GetStyle();
  Style.SetNormal(FSlateRoundedBoxBrush(Label.StartsWith(TEXT("Configure")) ? Primary : Surface, 14.0f));
  Style.SetHovered(FSlateRoundedBoxBrush(Hover, 14.0f)); Style.SetPressed(FSlateRoundedBoxBrush(Hover, 14.0f));
  Style.SetNormalPadding(FMargin(18, 14)); Style.SetPressedPadding(FMargin(18, 14)); Control->SetStyle(Style);
  auto* Text = WidgetTree->ConstructWidget<UTextBlock>(); Text->SetText(FText::FromString(Label)); Text->SetColorAndOpacity(Foreground);
  Control->AddChild(Text); auto* Slot = Panel->AddChildToVerticalBox(Control); Slot->SetPadding(FMargin(0, 6));
}
UEditableTextBox* UNuxieLabWidget::Input(const FString& Hint, const FString& Value) {
  auto* Control = WidgetTree->ConstructWidget<UEditableTextBox>(); Control->SetHintText(FText::FromString(Hint)); Control->SetText(FText::FromString(Value));
  FEditableTextBoxStyle Style = Control->GetWidgetStyle();
  Style.SetBackgroundImageNormal(FSlateRoundedBoxBrush(Surface, 14.0f)); Style.SetBackgroundImageHovered(FSlateRoundedBoxBrush(Hover, 14.0f)); Style.SetBackgroundImageFocused(FSlateRoundedBoxBrush(Hover, 14.0f)); Style.SetPadding(FMargin(16, 12)); Style.SetForegroundColor(Foreground); Control->SetWidgetStyle(Style);
  Panel->AddChildToVerticalBox(Control)->SetPadding(FMargin(0, 6)); return Control;
}
void UNuxieLabWidget::Log(const FString& Text) {
  UE_LOG(LogTemp, Display, TEXT("NuxieLab: %s"), *Text);
  Output->SetText(FText::FromString(Text + TEXT("\n\n") + Output->GetText().ToString().Left(6000)));
}
FNuxieCompletion UNuxieLabWidget::Completion(const FString& Label) {
  return FNuxieCompletion::CreateWeakLambda(this, [this, Label](const FNuxieResult& Result) { Log(Result.IsSuccess() ? Label + TEXT(": success") : Label + TEXT(": ") + Result.GetError().Message); });
}
TSharedRef<SWidget> UNuxieLabWidget::RebuildWidget() {
  Client = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNuxieSubsystem>() : nullptr;
  auto* Border = WidgetTree->ConstructWidget<UBorder>(); Border->SetBrushColor(Canvas); Border->SetPadding(FMargin(24, 48)); auto* Safe = WidgetTree->ConstructWidget<USafeZone>(); Safe->AddChild(Border); WidgetTree->RootWidget = Safe;
  auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(); Border->SetContent(Scroll);
  Panel = WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Panel);
  auto* Title = WidgetTree->ConstructWidget<UTextBlock>(); Title->SetText(FText::FromString(TEXT("Nuxie SDK Lab"))); Title->SetFont(FSlateFontInfo(Title->GetFont().FontObject, 28)); Panel->AddChildToVerticalBox(Title);
  const auto Options = UNuxieSettings::GetProjectOptions();
#if PLATFORM_IOS
  PublicKey = Input(TEXT("iOS public key"), Options.IOSPublicKey);
#else
  PublicKey = Input(TEXT("Android public key"), Options.AndroidPublicKey);
#endif
  Customer = Input(TEXT("Development customer ID"), TEXT("unreal-lab"));
  Feature = Input(TEXT("Metered feature ID"), TEXT("energy")); Entity = Input(TEXT("Entity ID (optional)"), TEXT("character-a")); ComparisonEntity = Input(TEXT("Unchanged comparison entity"), TEXT("character-b")); Event = Input(TEXT("Published trigger event"), TEXT("shop_opened"));
  Output = WidgetTree->ConstructWidget<UTextBlock>(); Output->SetAutoWrapText(true); Output->SetColorAndOpacity(Foreground);
  Save = Cast<UNuxieLabSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
  if (!Save) Save = NewObject<UNuxieLabSave>();
  Button(TEXT("Configure development client"), [this]() { auto Config = UNuxieSettings::GetProjectOptions(); Config.Environment = ENuxieEnvironment::Development; Config.LogLevel = ENuxieLogLevel::Debug; Config.IOSPublicKey = Config.AndroidPublicKey = PublicKey->GetText().ToString(); Client->Configure(Config, Completion(TEXT("Configure"))); });
  Button(TEXT("Configure external billing harness"), [this]() {
    auto Config = UNuxieSettings::GetProjectOptions(); Config.Environment = ENuxieEnvironment::Development;
    Config.IOSPublicKey = Config.AndroidPublicKey = PublicKey->GetText().ToString();
    Config.BillingMode = ENuxieBillingMode::External;
    Config.ExternalController = GetGameInstance()->GetSubsystem<UNuxieLabBilling>();
    Client->Configure(Config, Completion(TEXT("External harness (shutdown first to change billing mode)")));
  });
  Button(TEXT("Inspect external checkout"), [this]() {
    auto* Billing = GetGameInstance()->GetSubsystem<UNuxieLabBilling>();
    Log(FString::Printf(TEXT("Purchase pending: %s · restore pending: %s. This harness cannot grant store access."), Billing->Purchase && Billing->Purchase->IsPending() ? TEXT("yes") : TEXT("no"), Billing->Restore && Billing->Restore->IsPending() ? TEXT("yes") : TEXT("no")));
  });
  Button(TEXT("Cancel external checkout"), [this]() { Log(GetGameInstance()->GetSubsystem<UNuxieLabBilling>()->CancelPurchase() ? TEXT("Cancellation submitted once.") : TEXT("No active purchase, expired request, or duplicate completion.")); });
  Button(TEXT("Fail external restore"), [this]() { Log(GetGameInstance()->GetSubsystem<UNuxieLabBilling>()->FailRestore() ? TEXT("Restore failure submitted once.") : TEXT("No active restore, expired request, or duplicate completion.")); });
  Button(TEXT("Identify customer"), [this]() { Client->Identify(Customer->GetText().ToString(), FNuxieIdentityOptions(), Completion(TEXT("Identify"))); });
  Button(TEXT("Query entity remotely"), [this]() { FNuxieFeatureQuery Query; Query.EntityId = Entity->GetText().ToString(); Query.Policy = ENuxieFeaturePolicy::Remote; Client->CheckFeature(Feature->GetText().ToString(), Query, FNuxieFeatureCompletion::CreateWeakLambda(this, [this](const TNuxieResult<FNuxieFeatureAccess>& Result) { if (!Result.IsSuccess()) { Log(Result.GetError().Message); return; } const auto& Access = Result.GetValue(); Log(FString::Printf(TEXT("Allowed: %s · unlimited: %s · balance: %s"), Access.bAllowed ? TEXT("yes") : TEXT("no"), Access.bUnlimited ? TEXT("yes") : TEXT("no"), Access.bHasBalance ? *FString::SanitizeFloat(Access.Balance) : TEXT("absent"))); })); });
  Button(TEXT("Spend one / retry saved operation"), [this]() { Consume(); });
  Button(TEXT("Start a new operation"), [this]() { if (!Save->Command.OperationId.IsEmpty() && !Save->Resolved.Contains(Save->Command.OperationId)) { Log(TEXT("Resolve the saved operation before starting another.")); return; } Save->Command.OperationId.Reset(); if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0)) Log(TEXT("Could not save.")); else Log(TEXT("Next spend will save a new operation.")); });
  Button(TEXT("Validate debit and replay"), [this]() { Validate(); });
  Button(TEXT("Trigger published Experience"), [this]() { Client->Trigger(Event->GetText().ToString(), FNuxieProperties(), Completion(TEXT("Trigger accepted"))); });
  Button(TEXT("Dismiss Experience"), [this]() { Client->Dismiss(Completion(TEXT("Dismiss"))); });
  Button(TEXT("Use French locale"), [this]() { Client->SetLocale(TEXT("fr"), Completion(TEXT("Locale fr"))); });
  Button(TEXT("Restore device locale"), [this]() { Client->SetLocale(TEXT(""), Completion(TEXT("Device locale"))); });
  Button(TEXT("Reset to a new anonymous customer"), [this]() { Client->Reset(Completion(TEXT("Reset"))); });
  Button(TEXT("Travel to other map"), [this]() { UGameplayStatics::OpenLevel(this, FName(GetWorld()->GetMapName().Contains(TEXT("Second")) ? TEXT("Lab") : TEXT("LabSecond"))); });
  Button(TEXT("Shutdown"), [this]() { Client->Shutdown(Completion(TEXT("Shutdown"))); });
  Panel->AddChildToVerticalBox(Output)->SetPadding(FMargin(0, 18));
  if (!Client) return Super::RebuildWidget();
  Client->OnFeaturesChanged.RemoveAll(this); Client->OnActivity.RemoveAll(this); Client->OnAppAction.RemoveAll(this);
  Client->OnFeaturesChanged.AddDynamic(this, &UNuxieLabWidget::FeaturesChanged);
  Client->OnActivity.AddDynamic(this, &UNuxieLabWidget::Activity);
  Client->OnAppAction.AddDynamic(this, &UNuxieLabWidget::AppAction);
  FeaturesChanged(Client->GetFeatureSnapshot());
  return Super::RebuildWidget();
}
void UNuxieLabWidget::NativeDestruct() { if (Client) { Client->OnFeaturesChanged.RemoveAll(this); Client->OnActivity.RemoveAll(this); Client->OnAppAction.RemoveAll(this); } Super::NativeDestruct(); }
void UNuxieLabWidget::FeaturesChanged(const FNuxieFeatureSnapshot& Snapshot) { Log(FString::Printf(TEXT("%s · customer %s · generation %s · revision %s"), *StaticEnum<ENuxieFeatureStateKind>()->GetNameStringByValue(static_cast<int64>(Snapshot.Kind)), *Snapshot.CustomerId, *Snapshot.IdentityGeneration, *Snapshot.Revision)); }
void UNuxieLabWidget::Activity(const FNuxieActivity& Value) { Log(TEXT("Activity: ") + Value.Name); }
void UNuxieLabWidget::AppAction(const FNuxieAppAction& Value) { Log(TEXT("App action: ") + Value.Name + TEXT(" · ") + FString::Printf(TEXT("%d typed payload values"), Value.Payload.Num())); }
void UNuxieLabWidget::Consume() {
  if (Save->Command.OperationId.IsEmpty()) {
    Save->Customer = Client->GetFeatureSnapshot().CustomerId;
    if (Save->Customer.IsEmpty()) { Log(TEXT("Wait for customer identity before saving an action.")); return; }
    Save->Feature = Feature->GetText().ToString(); Save->Command.EntityId = Entity->GetText().ToString(); Save->Command.Quantity = 1; Save->Command.OperationId = FGuid::NewGuid().ToString();
    if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0)) { Save->Command.OperationId.Reset(); Log(TEXT("Save failed; no consumption was sent.")); return; }
  }
  if (Save->Customer != Client->GetFeatureSnapshot().CustomerId) { Log(TEXT("The saved operation belongs to another customer. Reidentify that customer before retrying.")); return; }
  bOperationPending = true;
  Client->ConsumeFeature(Save->Feature, Save->Command, FNuxieConsumeCompletion::CreateWeakLambda(this, [this](const TNuxieResult<FNuxieUsageReceipt>& Result) {
    bOperationPending = false;
    if (!Result.IsSuccess()) { Log(TEXT("Pending operation retained: ") + Result.GetError().Message); return; }
    const auto& Receipt = Result.GetValue();
    if (Receipt.CustomerId != Save->Customer || Receipt.OperationId != Save->Command.OperationId) { Log(TEXT("Receipt ownership mismatch.")); return; }
    if (Receipt.bAccepted && !Save->Applied.Contains(Receipt.OperationId)) {
      Save->Applied.Add(Receipt.OperationId);
      Save->Resolved.AddUnique(Receipt.OperationId);
      if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0)) { Save->Applied.Remove(Receipt.OperationId); Save->Resolved.Remove(Receipt.OperationId); Log(TEXT("Could not persist gameplay application; retry the same operation.")); return; }
      // The saved Applied set is this Lab's gameplay effect: one earned action per accepted operation.
    }
    if (!Receipt.bAccepted) { Save->Resolved.AddUnique(Receipt.OperationId); if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0)) { Save->Resolved.Remove(Receipt.OperationId); Log(TEXT("Could not persist the denied outcome; retry the saved operation.")); return; } }
    Log(FString::Printf(TEXT("%s · replay %s · applied actions %d · operation %s"), *Receipt.Code, Receipt.bIdempotentReplay ? TEXT("yes") : TEXT("no"), Save->Applied.Num(), *Receipt.OperationId));
  }));
}
void UNuxieLabWidget::ValidationFinished(bool bPassed, const FString& Message) {
  bOperationPending = false;
  auto Report = MakeShared<FJsonObject>();
  Report->SetBoolField(TEXT("passed"), bPassed);
  Report->SetStringField(TEXT("message"), Message);
  Report->SetStringField(TEXT("timestamp"), FDateTime::UtcNow().ToIso8601());
  Report->SetStringField(TEXT("customerId"), Save->Customer);
  Report->SetStringField(TEXT("featureId"), Save->Feature);
  Report->SetStringField(TEXT("entityId"), Save->Command.EntityId);
  Report->SetStringField(TEXT("comparisonEntityId"), ComparisonEntity->GetText().ToString());
  Report->SetStringField(TEXT("operationId"), Save->Command.OperationId);
  FString Json;
  FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
  const FString Directory = FPaths::ProjectSavedDir() / TEXT("NuxieLab");
  IFileManager::Get().MakeDirectory(*Directory, true);
  const bool bSaved = FFileHelper::SaveStringToFile(Json, *(Directory / TEXT("validation.json")));
  Log((bPassed ? TEXT("PASS: ") : TEXT("FAIL: ")) + Message + (bSaved ? TEXT(" Report: Saved/NuxieLab/validation.json") : TEXT(" Could not save the validation report.")));
}
void UNuxieLabWidget::Validate() {
  if (!Save->Command.OperationId.IsEmpty() && !Save->Resolved.Contains(Save->Command.OperationId)) { Log(TEXT("Resolve the saved operation before validation.")); return; }
  const FString CustomerId = Client->GetFeatureSnapshot().CustomerId;
  if (CustomerId.IsEmpty()) { Log(TEXT("Configure and identify before validation.")); return; }
  const FString FeatureId = Feature->GetText().ToString();
  FNuxieFeatureQuery Query; Query.Policy = ENuxieFeaturePolicy::Remote; Query.EntityId = Entity->GetText().ToString();
  FNuxieFeatureQuery OtherQuery = Query; OtherQuery.EntityId = ComparisonEntity->GetText().ToString();
  if (Query.EntityId.IsEmpty() || OtherQuery.EntityId.IsEmpty() || Query.EntityId == OtherQuery.EntityId) { Log(TEXT("Validation requires two different entity IDs.")); return; }
  bOperationPending = true;
  Client->CheckFeature(FeatureId, Query, FNuxieFeatureCompletion::CreateWeakLambda(this, [this, CustomerId, FeatureId, Query, OtherQuery](const TNuxieResult<FNuxieFeatureAccess>& Before) {
    if (!Before.IsSuccess()) { ValidationFinished(false, TEXT("Initial entity query: ") + Before.GetError().Message); return; }
    const auto Initial = Before.GetValue();
    if (!Initial.bHasBalance || Initial.bUnlimited || Initial.Balance < 1) { ValidationFinished(false, TEXT("The spending entity needs a finite balance of at least one.")); return; }
    Client->CheckFeature(FeatureId, OtherQuery, FNuxieFeatureCompletion::CreateWeakLambda(this, [this, CustomerId, FeatureId, Query, OtherQuery, Initial](const TNuxieResult<FNuxieFeatureAccess>& OtherBefore) {
      if (!OtherBefore.IsSuccess()) { ValidationFinished(false, TEXT("Initial comparison query: ") + OtherBefore.GetError().Message); return; }
      const auto OtherInitial = OtherBefore.GetValue();
      if (!OtherInitial.bHasBalance || OtherInitial.bUnlimited) { ValidationFinished(false, TEXT("The comparison entity needs a finite balance.")); return; }
      if (Client->GetFeatureSnapshot().CustomerId != CustomerId) { ValidationFinished(false, TEXT("Identity changed before the debit.")); return; }
      Save->Customer = CustomerId; Save->Feature = FeatureId; Save->Command = FNuxieFeatureCommand(); Save->Command.EntityId = Query.EntityId; Save->Command.OperationId = FGuid::NewGuid().ToString();
      if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0)) { Save->Command.OperationId.Reset(); ValidationFinished(false, TEXT("Cannot save the pending operation; no debit was sent.")); return; }
      const auto Command = Save->Command;
      Client->ConsumeFeature(FeatureId, Command, FNuxieConsumeCompletion::CreateWeakLambda(this, [this, CustomerId, FeatureId, Query, OtherQuery, Initial, OtherInitial, Command](const TNuxieResult<FNuxieUsageReceipt>& First) {
        if (!First.IsSuccess()) { ValidationFinished(false, TEXT("Debit unresolved; retry the saved ID: ") + First.GetError().Message); return; }
        const auto Receipt = First.GetValue();
        if (Receipt.CustomerId != CustomerId || Receipt.OperationId != Command.OperationId) { ValidationFinished(false, TEXT("Receipt ownership differs.")); return; }
        if (!Receipt.bAccepted || Receipt.bIdempotentReplay) {
          // A denied outcome is resolved, but never a gameplay reward.
          if (!Receipt.bAccepted) { Save->Resolved.AddUnique(Command.OperationId); if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0)) Save->Resolved.Remove(Command.OperationId); }
          ValidationFinished(false, TEXT("Expected a newly accepted debit; received ") + Receipt.Code); return;
        }
        Save->Applied.AddUnique(Command.OperationId); Save->Resolved.AddUnique(Command.OperationId);
        if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0)) { Save->Applied.Remove(Command.OperationId); Save->Resolved.Remove(Command.OperationId); ValidationFinished(false, TEXT("Could not save gameplay application; retry the saved ID.")); return; }
        Client->ConsumeFeature(FeatureId, Command, FNuxieConsumeCompletion::CreateWeakLambda(this, [this, FeatureId, Query, OtherQuery, Initial, OtherInitial, Command, Receipt](const TNuxieResult<FNuxieUsageReceipt>& Replay) {
          if (!Replay.IsSuccess()) { ValidationFinished(false, TEXT("Replay unresolved: ") + Replay.GetError().Message); return; }
          const auto Second = Replay.GetValue();
          if (!Second.bAccepted || !Second.bIdempotentReplay || Second.OperationId != Receipt.OperationId || Second.CustomerId != Receipt.CustomerId || Second.bHasBalance != Receipt.bHasBalance || (Second.bHasBalance && Second.Balance != Receipt.Balance)) { ValidationFinished(false, TEXT("Replay changed the authoritative receipt.")); return; }
          Client->CheckFeature(FeatureId, Query, FNuxieFeatureCompletion::CreateWeakLambda(this, [this, FeatureId, OtherQuery, Initial, OtherInitial, Command](const TNuxieResult<FNuxieFeatureAccess>& After) {
            if (!After.IsSuccess()) { ValidationFinished(false, TEXT("Final balance unavailable: ") + After.GetError().Message); return; }
            const auto Access = After.GetValue();
            if (!Access.bHasBalance || Access.bUnlimited || Access.Balance != Initial.Balance - 1 || Save->Applied.FilterByPredicate([Command](const FString& Id) { return Id == Command.OperationId; }).Num() != 1) { ValidationFinished(false, TEXT("Balance delta or saved action count differs.")); return; }
            Client->CheckFeature(FeatureId, OtherQuery, FNuxieFeatureCompletion::CreateWeakLambda(this, [this, OtherInitial](const TNuxieResult<FNuxieFeatureAccess>& OtherAfter) {
              if (!OtherAfter.IsSuccess()) { ValidationFinished(false, TEXT("Final comparison unavailable: ") + OtherAfter.GetError().Message); return; }
              const auto Access = OtherAfter.GetValue();
              const bool bUnchanged = Access.bHasBalance && !Access.bUnlimited && Access.Balance == OtherInitial.Balance && Access.bAllowed == OtherInitial.bAllowed;
              ValidationFinished(bUnchanged, bUnchanged ? TEXT("One debit, one saved action, exact replay, and unchanged second entity.") : TEXT("The second entity changed."));
            }));
          }));
        }));
      }));
    }));
  }));
}

void UNuxieLabWidget::NativeConstruct() {
  Super::NativeConstruct();
#if UE_BUILD_DEVELOPMENT
  // Explicit, local opt-in for device validation; no private SDK hooks or synthetic grants.
  const FString Path = FPaths::ProjectSavedDir() / TEXT("NuxieLab/auto.json");
  Log(TEXT("Development runner settings: ") + Path);
  FString Json; TSharedPtr<FJsonObject> Settings;
  if (!FFileHelper::LoadFileToString(Json, *Path)) return;
  if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Settings) || !Settings) { Log(TEXT("Invalid development runner JSON.")); return; }
  FString Key, CustomerId, FeatureId, EntityId, OtherEntityId;
  if (!Settings->TryGetStringField(TEXT("publicKey"), Key) || !Settings->TryGetStringField(TEXT("customerId"), CustomerId) || !Settings->TryGetStringField(TEXT("featureId"), FeatureId) || !Settings->TryGetStringField(TEXT("entityId"), EntityId) || !Settings->TryGetStringField(TEXT("comparisonEntityId"), OtherEntityId)) { Log(TEXT("Development runner requires publicKey, customerId, featureId, entityId, comparisonEntityId.")); return; }
  PublicKey->SetText(FText::FromString(Key)); Customer->SetText(FText::FromString(CustomerId)); Feature->SetText(FText::FromString(FeatureId)); Entity->SetText(FText::FromString(EntityId)); ComparisonEntity->SetText(FText::FromString(OtherEntityId));
  FNuxieOptions Options; Options.IOSPublicKey = Options.AndroidPublicKey = Key; Options.Environment = ENuxieEnvironment::Development; Options.LogLevel = ENuxieLogLevel::Debug;
  bOperationPending = true;
  Client->Configure(Options, FNuxieCompletion::CreateWeakLambda(this, [this, CustomerId](const FNuxieResult& Setup) {
    if (!Setup.IsSuccess()) { ValidationFinished(false, TEXT("Configure: ") + Setup.GetError().Message); return; }
    Log(TEXT("Automatic configure succeeded."));
    Client->Identify(CustomerId, FNuxieIdentityOptions(), FNuxieCompletion::CreateWeakLambda(this, [this](const FNuxieResult& Identified) {
      bOperationPending = false;
      if (!Identified.IsSuccess()) { ValidationFinished(false, TEXT("Identify: ") + Identified.GetError().Message); return; }
      Log(TEXT("Automatic identify succeeded.")); Validate();
    }));
  }));
#endif
}

#include "NuxieSubsystem.h"
#include "NuxieSession.h"
#include "NuxiePurchaseController.h"
namespace {
auto Args() { return MakeShared<FJsonObject>(); }
FNuxieError Invalid(const FString& Message) { return NuxieWire::Error(ENuxieErrorCode::InvalidArgument, Message); }
FNuxieError NotConfigured() { return NuxieWire::Error(ENuxieErrorCode::NotConfigured, TEXT("Configure Nuxie first.")); }
void Fail(FNuxieCompletion Completion, FNuxieError Error) { FNuxieSession::Defer([Completion, Error]() { FNuxieResult Result; Result.Error = Error; Completion.ExecuteIfBound(Result); }); }
FNuxieSession::FReply VoidReply(FNuxieCompletion Completion) { return [Completion](NuxieWire::FObject, FNuxieError Error) { FNuxieResult Result; Result.Error = Error; Completion.ExecuteIfBound(Result); }; }
bool Nonempty(const FString& Value) { return !Value.TrimStartAndEnd().IsEmpty(); }
}
void UNuxieSubsystem::Initialize(FSubsystemCollectionBase& Collection) { Super::Initialize(Collection); CompletionLifetime = MakeShared<bool>(true); }
void UNuxieSubsystem::Deinitialize() {
  *CompletionLifetime = false;
  OnDeinitializing.Broadcast();
  OnDeinitializing.Clear();
  InvalidateCheckout();
  if (Session) Session->DetachOwner();
  PurchaseController = nullptr;
  Session.Reset();
  Super::Deinitialize();
}
void UNuxieSubsystem::Configure(const FNuxieOptions& Options, FNuxieCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion)); FNuxieSession::Configure(this, Options, MoveTemp(Completion)); }
void UNuxieSubsystem::Shutdown(FNuxieCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion)); FNuxieSession::ShutdownFor(this, MoveTemp(Completion)); }
void UNuxieSubsystem::Identify(const FString& CustomerId, const FNuxieIdentityOptions& Options, FNuxieCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion));
  if (!Nonempty(CustomerId)) { Fail(Completion, Invalid(TEXT("Customer ID must not be empty."))); return; }
  if (!Session) { Fail(Completion, NotConfigured()); return; }
  auto Properties = Args(); Properties->SetObjectField(TEXT("properties"), NuxieWire::Object(Options.Properties.ToJson())); Properties->SetObjectField(TEXT("propertiesSetOnce"), NuxieWire::Object(Options.PropertiesSetOnce.ToJson()));
  auto Arguments = Args(); Arguments->SetStringField(TEXT("customerId"), CustomerId); Arguments->SetStringField(TEXT("properties"), NuxieWire::Json(Properties));
  Session->ChangeIdentity(TEXT("identify"), Arguments, MoveTemp(Completion));
}
void UNuxieSubsystem::Reset(FNuxieCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion)); if (Session) Session->ChangeIdentity(TEXT("reset"), Args(), MoveTemp(Completion)); else Fail(Completion, NotConfigured()); }
void UNuxieSubsystem::GetIdentity(FNuxieIdentityCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion));
  if (!Session) { FNuxieSession::Defer([Completion]() { Completion.ExecuteIfBound(TNuxieResult<FNuxieIdentity>::Failure(NotConfigured())); }); return; }
  Session->Call(TEXT("getIdentity"), Args(), false, [Completion](NuxieWire::FObject Value, FNuxieError Error) {
    FNuxieIdentity Identity;
    if (Error.Code == ENuxieErrorCode::None && !NuxieWire::Identity(Value, Identity)) Error = NuxieWire::Error(ENuxieErrorCode::InvalidResponse, TEXT("Invalid identity response."));
    Completion.ExecuteIfBound(Error.Code == ENuxieErrorCode::None ? TNuxieResult<FNuxieIdentity>::Success(Identity) : TNuxieResult<FNuxieIdentity>::Failure(Error));
  });
}
void UNuxieSubsystem::CheckFeature(const FString& FeatureId, const FNuxieFeatureQuery& Query, FNuxieFeatureCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion));
  FNuxieError Error;
  if (!Nonempty(FeatureId) || !nuxie::quantity(Query.RequiredBalance)) Error = Invalid(TEXT("A feature ID and required balance between 1 and 2^53-1 are required."));
  else if (!Session) Error = NotConfigured();
  if (Error.Code != ENuxieErrorCode::None) { FNuxieSession::Defer([Completion, Error]() { Completion.ExecuteIfBound(TNuxieResult<FNuxieFeatureAccess>::Failure(Error)); }); return; }
  auto Options = Args(); Options->SetNumberField(TEXT("requiredBalance"), static_cast<double>(Query.RequiredBalance)); Options->SetStringField(TEXT("policy"), Query.Policy == ENuxieFeaturePolicy::Remote ? TEXT("remote") : TEXT("cacheFirst"));
  if (!Query.EntityId.IsEmpty()) Options->SetStringField(TEXT("entityId"), Query.EntityId);
  auto Arguments = Args(); Arguments->SetStringField(TEXT("featureId"), FeatureId); Arguments->SetStringField(TEXT("options"), NuxieWire::Json(Options));
  Session->Call(TEXT("hasFeature"), Arguments, false, [Completion](NuxieWire::FObject Value, FNuxieError Failure) {
    FNuxieFeatureAccess Access;
    if (Failure.Code == ENuxieErrorCode::None && !NuxieWire::Access(Value, Access)) Failure = NuxieWire::Error(ENuxieErrorCode::InvalidResponse, TEXT("Invalid feature response."));
    Completion.ExecuteIfBound(Failure.Code == ENuxieErrorCode::None ? TNuxieResult<FNuxieFeatureAccess>::Success(Access) : TNuxieResult<FNuxieFeatureAccess>::Failure(Failure));
  });
}
void UNuxieSubsystem::ConsumeFeature(const FString& FeatureId, const FNuxieFeatureCommand& Command, FNuxieConsumeCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion));
  FNuxieError Error;
  if (!Nonempty(FeatureId) || !Nonempty(Command.OperationId) || !nuxie::quantity(Command.Quantity)) Error = Invalid(TEXT("Feature ID, saved operation ID, and quantity between 1 and 2^53-1 are required."));
  else if (!Session) Error = NotConfigured();
  if (Error.Code != ENuxieErrorCode::None) { FNuxieSession::Defer([Completion, Error]() { Completion.ExecuteIfBound(TNuxieResult<FNuxieUsageReceipt>::Failure(Error)); }); return; }
  auto Options = Args(); Options->SetStringField(TEXT("operationId"), Command.OperationId); Options->SetNumberField(TEXT("quantity"), static_cast<double>(Command.Quantity));
  if (!Command.EntityId.IsEmpty()) Options->SetStringField(TEXT("entityId"), Command.EntityId);
  auto Arguments = Args(); Arguments->SetStringField(TEXT("featureId"), FeatureId); Arguments->SetStringField(TEXT("options"), NuxieWire::Json(Options));
  const FString Customer = Session->Identity.CustomerId;
  Session->Call(TEXT("consumeFeature"), Arguments, true, [Completion, Command, FeatureId, Customer](NuxieWire::FObject Value, FNuxieError Failure) {
    FNuxieUsageReceipt Receipt;
    if (Failure.Code == ENuxieErrorCode::None && (!NuxieWire::Receipt(Value, Receipt) || Receipt.CustomerId != Customer || Receipt.FeatureId != FeatureId || Receipt.OperationId != Command.OperationId || Receipt.Quantity != Command.Quantity)) Failure = NuxieWire::Error(ENuxieErrorCode::InvalidResponse, TEXT("Usage receipt does not match the admitted operation."));
    Completion.ExecuteIfBound(Failure.Code == ENuxieErrorCode::None ? TNuxieResult<FNuxieUsageReceipt>::Success(Receipt) : TNuxieResult<FNuxieUsageReceipt>::Failure(Failure));
  });
}
void UNuxieSubsystem::Trigger(const FString& EventName, const FNuxieProperties& Properties, FNuxieCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion));
  if (!Nonempty(EventName) || EventName.StartsWith(TEXT("$"))) { Fail(Completion, Invalid(TEXT("Use a nonempty customer event name without the reserved $ prefix."))); return; }
  if (!Session) { Fail(Completion, NotConfigured()); return; }
  auto Arguments = Args(); Arguments->SetStringField(TEXT("event"), EventName); Arguments->SetStringField(TEXT("properties"), Properties.ToJson());
  Session->Call(TEXT("trigger"), Arguments, false, VoidReply(MoveTemp(Completion)));
}
void UNuxieSubsystem::Dismiss(FNuxieCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion)); if (Session) Session->Call(TEXT("dismiss"), Args(), false, VoidReply(MoveTemp(Completion))); else Fail(Completion, NotConfigured()); }
void UNuxieSubsystem::SetLocale(const FString& Locale, FNuxieCompletion Completion) {
  Completion = GuardCompletion(MoveTemp(Completion));
  if (!Session) { Fail(Completion, NotConfigured()); return; }
  auto Arguments = Args();
  if (!Locale.IsEmpty()) Arguments->SetStringField(TEXT("locale"), Locale);
  Session->Call(TEXT("setLocaleIdentifier"), Arguments, false, VoidReply(MoveTemp(Completion)));
}
FNuxieStatus UNuxieSubsystem::GetStatus() const { return Session ? Session->Status : FNuxieStatus(); }
FNuxieFeatureSnapshot UNuxieSubsystem::GetFeatureSnapshot() const { return Session ? Session->Features : FNuxieFeatureSnapshot(); }
FNuxieFeatureState UNuxieSubsystem::GetFeatureState(const FString& FeatureId) const {
  return GetFeatureSnapshot().Select(FeatureId);
}
void UNuxieSubsystem::InvalidateCheckout() {
  for (auto Request : Purchases) if (Request) Request->bPending = false;
  for (auto Request : Restores) if (Request) Request->bPending = false;
  Purchases.Reset(); Restores.Reset();
}

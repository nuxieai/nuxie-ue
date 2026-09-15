#include "NuxieSession.h"
#include "NuxiePurchaseController.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformMisc.h"
#include "Containers/Queue.h"
#include <atomic>
namespace {
TSharedPtr<FNuxieSession> NativeOwner;
TQueue<TFunction<void()>, EQueueMode::Mpsc> DeferredCallbacks;
std::atomic<int32> DeferredCount{0};
std::atomic<bool> DeferredTickerPending{false};
uint64 DeferredFrame = MAX_uint64;
int32 DeferredDispatchedThisFrame = 0;
void ScheduleDeferredTicker() {
  if (DeferredTickerPending.exchange(true)) return;
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) {
    if (DeferredFrame != GFrameCounter) { DeferredFrame = GFrameCounter; DeferredDispatchedThisFrame = 0; }
    TFunction<void()> Callback;
    while (DeferredDispatchedThisFrame < 256 && DeferredCallbacks.Dequeue(Callback)) {
      ++DeferredDispatchedThisFrame;
      DeferredCount.fetch_sub(1);
      Callback();
    }
    if (DeferredCount.load() > 0) return true;
    DeferredTickerPending.store(false);
    // Cover an arrival racing the transition to idle.
    if (DeferredCount.load() > 0) ScheduleDeferredTicker();
    return false;
  }));
}
#if WITH_DEV_AUTOMATION_TESTS
TFunction<TUniquePtr<INuxieNativeTransport>()> TestTransport;
#endif
bool NativeAvailable() {
#if (PLATFORM_IOS || PLATFORM_ANDROID) && !UE_SERVER
  return true;
#elif WITH_DEV_AUTOMATION_TESTS
  return static_cast<bool>(TestTransport);
#else
  return false;
#endif
}
TUniquePtr<INuxieNativeTransport> TransportForSession() {
#if WITH_DEV_AUTOMATION_TESTS
  if (TestTransport) return TestTransport();
#endif
  return CreateNuxieNativeTransport();
}
NuxieWire::FObject Empty() { return MakeShared<FJsonObject>(); }
NuxieWire::FObject Child(const NuxieWire::FObject& Object, const FString& Key) {
  const NuxieWire::FObject* Value = nullptr;
  return Object && Object->TryGetObjectField(Key, Value) ? *Value : nullptr;
}
FNuxieError InvalidResponse() { return NuxieWire::Error(ENuxieErrorCode::InvalidResponse, TEXT("The native bridge returned an invalid response.")); }
FNuxieResult Result(FNuxieError Error) { FNuxieResult Value; Value.Error = MoveTemp(Error); return Value; }
}
FNuxieSession::FNuxieSession(UNuxieSubsystem* InOwner) : Owner(InOwner), Transport(TransportForSession()), SessionId(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens)) {}
FNuxieSession::~FNuxieSession() {
  FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
  FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
  FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
}
void FNuxieSession::Defer(TFunction<void()> Callback) {
  DeferredCount.fetch_add(1);
  DeferredCallbacks.Enqueue(MoveTemp(Callback));
  ScheduleDeferredTicker();
#if PLATFORM_ANDROID
  WakeNuxieAndroidDispatcher();
#endif
}
bool FNuxieSession::HasDeferredCallbacks() { return DeferredCount.load() > 0; }
bool FNuxieSession::HasNativeOwner() { check(IsInGameThread()); return NativeOwner.IsValid(); }
void FNuxieSession::Start() {
  TWeakPtr<FNuxieSession> Weak = AsShared();
  Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float Delta) { auto Self = Weak.Pin(); return Self && Self->Tick(Delta); }));
  BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddLambda([Weak]() { if (auto Self = Weak.Pin()) Self->bForeground = false; });
  ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddLambda([Weak]() { if (auto Self = Weak.Pin()) Self->bForeground = true; });
}
void FNuxieSession::SetStatus(ENuxieStatusKind Kind, FNuxieError Error) {
  Status.Kind = Kind; Status.Error = MoveTemp(Error);
  if (Owner.IsValid()) Owner->OnStatusChanged.Broadcast(Status);
}
void FNuxieSession::PublishFeatures() { if (Owner.IsValid()) Owner->OnFeaturesChanged.Broadcast(Features); }
void FNuxieSession::Configure(UNuxieSubsystem* InOwner, const FNuxieOptions& Options, FNuxieCompletion Completion) {
  check(IsInGameThread());
if (!NativeAvailable()) {
  Defer([Completion]() { Completion.ExecuteIfBound(Result(NuxieWire::Error(ENuxieErrorCode::UnsupportedPlatform, TEXT("Nuxie requires an iOS or Android client. Editor and server calls do not simulate customer access.")))); });
  return;
}
  auto Config = Empty();
#if PLATFORM_IOS
  const FString Key = Options.IOSPublicKey;
#else
  const FString Key = Options.AndroidPublicKey;
#endif
  const bool External = Options.BillingMode == ENuxieBillingMode::External;
  if (Key.TrimStartAndEnd().IsEmpty() || (External && (!Options.ExternalController || !Options.ExternalController->GetClass()->ImplementsInterface(UNuxiePurchaseController::StaticClass())))) {
    Defer([Completion]() { Completion.ExecuteIfBound(Result(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("Set the platform public key and provide a purchase controller for external billing.")))); }); return;
  }
  Config->SetNumberField(TEXT("contract"), 1);
  Config->SetStringField(TEXT("apiKey"), Key);
  Config->SetStringField(TEXT("environment"), Options.Environment == ENuxieEnvironment::Development ? TEXT("development") : TEXT("production"));
  Config->SetStringField(TEXT("logLevel"), StaticEnum<ENuxieLogLevel>()->GetNameStringByValue(static_cast<int64>(Options.LogLevel)).ToLower());
  if (!Options.Locale.IsEmpty()) Config->SetStringField(TEXT("localeIdentifier"), Options.Locale);
  Config->SetStringField(TEXT("purchaseHandlingMode"), External ? TEXT("observer") : TEXT("full"));
  Config->SetBoolField(TEXT("externalBilling"), External);
#if PLATFORM_IOS && !UE_BUILD_SHIPPING
  // The host build controls local development, including with a prepared Release bridge.
  if (Options.Environment == ENuxieEnvironment::Development) {
    const FString Endpoint = FPlatformMisc::GetEnvironmentVariable(TEXT("NUXIE_UNREAL_API_ENDPOINT"));
    if (!Endpoint.IsEmpty()) Config->SetStringField(TEXT("testingApiEndpoint"), Endpoint);
  }
#endif
  const FString Configuration = NuxieWire::Json(Config);
  if (NativeOwner) {
    auto Existing = NativeOwner;
    FNuxieError Error;
    if (Existing->Owner.Get() != InOwner) Error = NuxieWire::Error(ENuxieErrorCode::SessionInUse, TEXT("Another game instance owns the native SDK. Shutdown that instance first."));
    else if (Existing->Configuration != Configuration || Existing->Controller.Get() != Options.ExternalController.Get()) Error = NuxieWire::Error(ENuxieErrorCode::AlreadyConfigured, TEXT("Shutdown before changing configuration."));
    else if (Existing->Status.Kind == ENuxieStatusKind::Configuring) { Existing->ConfigureWaiters.Add(MoveTemp(Completion)); return; }
    else if (Existing->Status.Kind != ENuxieStatusKind::Ready) Error = NuxieWire::Error(ENuxieErrorCode::LifecycleBusy, TEXT("Shutdown must finish before configuring."));
    Defer([Completion, Error]() { Completion.ExecuteIfBound(Result(Error)); }); return;
  }
  auto Self = MakeShared<FNuxieSession>(InOwner);
  if (Self->Transport->ContractVersion() != 1) {
    Defer([Completion]() { Completion.ExecuteIfBound(Result(NuxieWire::Error(ENuxieErrorCode::IncompatibleBridge, TEXT("Prepare and package native artifacts matching this SDK.")))); }); return;
  }
  NativeOwner = Self; InOwner->Session = Self;
  InOwner->PurchaseController = External ? Options.ExternalController : nullptr;
  Self->Controller = Options.ExternalController;
  Self->Configuration = Configuration;
  Self->ConfigureWaiters.Add(MoveTemp(Completion));
  Self->Start(); Self->SetStatus(ENuxieStatusKind::Configuring);
  // Blueprint listeners may shut down synchronously during any broadcast.
  if (Self->Status.Kind != ENuxieStatusKind::Configuring) return;
  Config->SetStringField(TEXT("session"), Self->SessionId);
  auto Args = Empty(); Args->SetStringField(TEXT("configuration"), NuxieWire::Json(Config));
  Self->Send(TEXT("configure"), Args, true, [Self](NuxieWire::FObject Value, FNuxieError Error) {
    FString Session; double Contract = 0;
    if (Error.Code == ENuxieErrorCode::None && (!Value || !Value->TryGetStringField(TEXT("session"), Session) || Session != Self->SessionId || !Value->TryGetNumberField(TEXT("contract"), Contract) || Contract != 1 || !Self->AdoptIdentity(Value))) Error = InvalidResponse();
    if (Self->Status.Kind != ENuxieStatusKind::Configuring) Error = NuxieWire::Error(ENuxieErrorCode::SDKShutdown, TEXT("Configuration was interrupted by shutdown."));
    else Self->SetStatus(Error.Code == ENuxieErrorCode::None ? ENuxieStatusKind::Ready : ENuxieStatusKind::Failed, Error);
    auto Waiters = MoveTemp(Self->ConfigureWaiters);
    for (auto& Waiter : Waiters) Waiter.ExecuteIfBound(Result(Error));
    // A failed or timed-out setup may still own native state. Keep the lease until shutdown acknowledges cleanup.
  });
}
bool FNuxieSession::Send(const FString& Method, NuxieWire::FObject Arguments, bool bDurable, FReply Reply) {
  const bool bCheckout = Method == TEXT("completePurchase") || Method == TEXT("completeRestore");
  const bool bLifecycle = Method == TEXT("configure") || Method == TEXT("shutdown") || Method == TEXT("identify") || Method == TEXT("reset");
  const int32 Capacity = bLifecycle ? 132 : bCheckout ? 128 : 64;
  if (Replies.Num() >= Capacity) {
    Defer([Reply = MoveTemp(Reply)]() mutable { Reply(nullptr, NuxieWire::Error(ENuxieErrorCode::Overloaded, TEXT("Native admission is full. Wait for a completion before retrying."))); });
    return false;
  }
  const FString Id = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
  Arguments->SetStringField(TEXT("session"), SessionId);
  auto Envelope = Empty(); Envelope->SetStringField(TEXT("requestId"), Id); Envelope->SetStringField(TEXT("method"), Method); Envelope->SetObjectField(TEXT("arguments"), Arguments);
  Ledger.admit(TCHAR_TO_UTF8(*Id), {IdentityEpoch, bDurable, ActiveTime + 90.0});
  Replies.Add(Id, MoveTemp(Reply));
  if (!Transport->Submit(NuxieWire::Json(Envelope))) {
    auto Self = AsShared();
    Defer([Self, Id]() { Self->Finish(Id, nullptr, NuxieWire::Error(ENuxieErrorCode::NativeError, TEXT("Native dispatch failed."))); });
    return false;
  }
  return true;
}
void FNuxieSession::Call(const FString& Method, NuxieWire::FObject Arguments, bool bDurable, FReply Reply) {
  check(IsInGameThread());
  FNuxieError Error;
  if (Status.Kind != ENuxieStatusKind::Ready) Error = NuxieWire::Error(ENuxieErrorCode::NotConfigured, TEXT("Wait for Configure to succeed before calling Nuxie."));
  else if (bChangingIdentity) Error = NuxieWire::Error(ENuxieErrorCode::LifecycleBusy, TEXT("Wait for the identity change to finish."));
  if (Error.Code != ENuxieErrorCode::None) { Defer([Reply = MoveTemp(Reply), Error]() mutable { Reply(nullptr, Error); }); return; }
  Send(Method, Arguments, bDurable, MoveTemp(Reply));
}
void FNuxieSession::Finish(const FString& Id, NuxieWire::FObject Value, FNuxieError Error) {
  auto Request = Ledger.take(TCHAR_TO_UTF8(*Id));
  FReply Reply;
  if (!Request || !Replies.RemoveAndCopyValue(Id, Reply)) return;
  if (!nuxie::RequestLedger::deliverable(*Request, IdentityEpoch)) Error = NuxieWire::Error(ENuxieErrorCode::IdentityChanged, TEXT("The customer changed while this request was pending."));
  Reply(Value, Error);
}
bool FNuxieSession::Tick(float Delta) {
  check(IsInGameThread());
  if (bForeground) ActiveTime += FMath::Min(static_cast<double>(Delta), 1.0);
  FString Message;
  for (int32 Count = 0; Count < 256 && NativeOwner.Get() == this && Transport->Poll(Message); ++Count) {
    auto Envelope = NuxieWire::Object(Message);
    if (!Envelope) { if (Owner.IsValid()) Owner->OnError.Broadcast(InvalidResponse()); continue; }
    FString Id;
    if (Envelope->TryGetStringField(TEXT("requestId"), Id)) {
      auto Error = Child(Envelope, TEXT("error"));
      FString Json;
      auto Value = Envelope->TryGetStringField(TEXT("result"), Json) ? NuxieWire::Object(Json) : nullptr;
      Finish(Id, Value, Error ? NuxieWire::NativeError(Error) : FNuxieError());
    } else Event(Envelope);
  }
  if (NativeOwner.Get() != this) return false;
  for (const auto& Id : Ledger.expired(ActiveTime)) Finish(UTF8_TO_TCHAR(Id.c_str()), nullptr, NuxieWire::Error(ENuxieErrorCode::OperationTimeout, TEXT("The operation timed out. Reconcile durable usage with the same operation ID.")));
  if (Owner.IsValid()) {
    const double Now = FDateTime::UtcNow().ToUnixTimestamp() * 1000.0;
    Owner->Purchases.RemoveAll([Now](const auto& Request) { return !Request || !Request->IsPendingAt(Now); });
    Owner->Restores.RemoveAll([Now](const auto& Request) { return !Request || !Request->IsPendingAt(Now); });
  }
  return true;
}
bool FNuxieSession::AdoptIdentity(const NuxieWire::FObject& Value) {
  FNuxieIdentity NextIdentity; FNuxieFeatureSnapshot NextFeatures;
  if (!NuxieWire::Identity(Child(Value, TEXT("identity")), NextIdentity) || !NuxieWire::Snapshot(Child(Value, TEXT("snapshot")), NextFeatures)) return false;
  NextFeatures.CustomerId = NextIdentity.CustomerId;
  Identity = MoveTemp(NextIdentity); Features = MoveTemp(NextFeatures);
  if (Owner.IsValid()) Owner->OnIdentityChanged.Broadcast(Identity);
  if (Status.Kind != ENuxieStatusKind::Configuring && Status.Kind != ENuxieStatusKind::Ready) return false;
  PublishFeatures(); return true;
}
void FNuxieSession::ChangeIdentity(const FString& Method, NuxieWire::FObject Arguments, FNuxieCompletion Completion) {
  if (Status.Kind != ENuxieStatusKind::Ready || bChangingIdentity) {
    Defer([Completion]() { Completion.ExecuteIfBound(Result(NuxieWire::Error(ENuxieErrorCode::LifecycleBusy, TEXT("Configure and finish any previous identity change first.")))); }); return;
  }
  if (Replies.Num() >= 64) {
    Defer([Completion]() { Completion.ExecuteIfBound(Result(NuxieWire::Error(ENuxieErrorCode::Overloaded, TEXT("Wait for pending operations before changing identity.")))); }); return;
  }
  ++IdentityEpoch; bChangingIdentity = true;
  Features = FNuxieFeatureSnapshot(); PublishFeatures();
  if (Owner.IsValid()) Owner->InvalidateCheckout();
  if (Status.Kind != ENuxieStatusKind::Ready || !bChangingIdentity) {
    Defer([Completion]() { Completion.ExecuteIfBound(Result(NuxieWire::Error(ENuxieErrorCode::SDKShutdown, TEXT("The identity change was interrupted by shutdown.")))); }); return;
  }
  auto Self = AsShared();
  Send(Method, Arguments, true, [Self, Completion](NuxieWire::FObject Value, FNuxieError Error) {
    Self->bChangingIdentity = false;
    if (Error.Code == ENuxieErrorCode::None && !Self->AdoptIdentity(Value)) Error = InvalidResponse();
    if (Self->Status.Kind != ENuxieStatusKind::Ready) Error = NuxieWire::Error(ENuxieErrorCode::SDKShutdown, TEXT("The identity change was interrupted by shutdown."));
    else if (Error.Code != ENuxieErrorCode::None) Self->SetStatus(ENuxieStatusKind::Failed, Error);
    Completion.ExecuteIfBound(Result(Error));
  });
}
void FNuxieSession::ShutdownFor(UNuxieSubsystem* InOwner, FNuxieCompletion Completion) {
  if (InOwner->Session) { InOwner->Session->Shutdown(MoveTemp(Completion)); return; }
  if (NativeOwner && !NativeOwner->Owner.IsValid()) { NativeOwner->Shutdown(MoveTemp(Completion)); return; }
  const auto Error = NativeOwner ? NuxieWire::Error(ENuxieErrorCode::SessionInUse, TEXT("Another live game instance owns Nuxie.")) : FNuxieError();
  Defer([Completion, Error]() { Completion.ExecuteIfBound(Result(Error)); });
}
void FNuxieSession::Shutdown(FNuxieCompletion Completion) {
  check(IsInGameThread());
  if (Status.Kind == ENuxieStatusKind::Unconfigured) { Defer([Completion]() { Completion.ExecuteIfBound(FNuxieResult()); }); return; }
  ShutdownWaiters.Add(MoveTemp(Completion));
  if (Status.Kind == ENuxieStatusKind::ShuttingDown) return;
  SetStatus(ENuxieStatusKind::ShuttingDown);
  if (Owner.IsValid()) Owner->InvalidateCheckout();
  for (const auto& Id : Ledger.all()) Finish(UTF8_TO_TCHAR(Id.c_str()), nullptr, NuxieWire::Error(ENuxieErrorCode::SDKShutdown, TEXT("The SDK is shutting down.")));
  auto SetupWaiters = MoveTemp(ConfigureWaiters);
  for (auto& Waiter : SetupWaiters) Waiter.ExecuteIfBound(Result(NuxieWire::Error(ENuxieErrorCode::SDKShutdown, TEXT("Configuration was interrupted by shutdown."))));
  ++IdentityEpoch; bChangingIdentity = false;
  auto Self = AsShared();
  Send(TEXT("shutdown"), Empty(), true, [Self](NuxieWire::FObject, FNuxieError Error) {
    // sessionExpired means native setup already relinquished this session.
    if (Error.NativeCode == TEXT("sessionExpired")) Error = FNuxieError();
    if (Error.Code == ENuxieErrorCode::None) {
      Self->Features = FNuxieFeatureSnapshot(); Self->Identity = FNuxieIdentity();
      if (NativeOwner == Self) NativeOwner.Reset();
      if (Self->Owner.IsValid()) { Self->Owner->PurchaseController = nullptr; Self->Owner->Session.Reset(); }
      Self->SetStatus(ENuxieStatusKind::Unconfigured);
      if (Self->Owner.IsValid() && !Self->Owner->Session) Self->PublishFeatures();
    } else Self->SetStatus(ENuxieStatusKind::Failed, Error);
    auto Waiters = MoveTemp(Self->ShutdownWaiters);
    for (auto& Waiter : Waiters) Waiter.ExecuteIfBound(Result(Error));
  });
}
bool FNuxieSession::CompleteCheckout(const FString& Method, const FString& RequestId, const FString& ResultJson) {
  if (Status.Kind != ENuxieStatusKind::Ready || bChangingIdentity) return false;
  auto Args = Empty(); Args->SetStringField(TEXT("requestId"), RequestId); Args->SetStringField(TEXT("result"), ResultJson);
  return Send(Method, Args, true, [Weak = Owner](NuxieWire::FObject, FNuxieError Error) { if (Weak.IsValid() && Error.Code != ENuxieErrorCode::None) Weak->OnError.Broadcast(Error); });
}
void FNuxieSession::Event(const NuxieWire::FObject& Envelope) {
  FString Session, Name;
  if (!Owner.IsValid() || !Envelope->TryGetStringField(TEXT("session"), Session) || Session != SessionId || !Envelope->TryGetStringField(TEXT("name"), Name)) return;
  auto Payload = Child(Envelope, TEXT("payload"));
  if (!Payload || Status.Kind != ENuxieStatusKind::Ready) return;
  if (!NuxieWire::EventMatchesIdentity(Envelope, SessionId, Features.IdentityGeneration, bChangingIdentity)) return;
  if (Name == TEXT("overflow")) {
    Owner->OnError.Broadcast(NuxieWire::Error(ENuxieErrorCode::Overloaded, TEXT("The native activity buffer overflowed. Some observational events were dropped; replies and checkout requests retain reserved capacity.")));
  } else if (Name == TEXT("features")) {
    FNuxieFeatureSnapshot Next;
    if (bChangingIdentity || !NuxieWire::Snapshot(Payload, Next) || Next.IdentityGeneration != Features.IdentityGeneration) return;
    const auto Revision = nuxie::counter(TCHAR_TO_UTF8(*Next.Revision));
    const auto Previous = nuxie::counter(TCHAR_TO_UTF8(*Features.Revision));
    if (!Revision || !Previous || *Revision <= *Previous) return;
    Next.CustomerId = Identity.CustomerId; Features = MoveTemp(Next); PublishFeatures();
  } else if (Name == TEXT("activity")) {
    FNuxieActivity Activity; double Timestamp, Received;
    if (!Payload->TryGetStringField(TEXT("id"), Activity.Id) || !Payload->TryGetStringField(TEXT("name"), Activity.Name) || !Payload->TryGetNumberField(TEXT("timestampMs"), Timestamp) || !Payload->TryGetNumberField(TEXT("receivedAtMs"), Received) || !FMath::IsFinite(Timestamp) || !FMath::IsFinite(Received) || FMath::Abs(Timestamp) > 9007199254740991.0 || FMath::Abs(Received) > 9007199254740991.0) return;
    Activity.TimestampMs = static_cast<int64>(Timestamp); Activity.ReceivedAtMs = static_cast<int64>(Received);
    if (!NuxieWire::Scalars(Child(Payload, TEXT("properties")), Activity.Properties)) { Owner->OnError.Broadcast(InvalidResponse()); return; }
    Owner->OnActivity.Broadcast(Activity);
  } else if (Name == TEXT("appAction")) {
    FNuxieAppAction Action;
    auto Experience = Child(Payload, TEXT("experience"));
    if (!Payload->TryGetStringField(TEXT("name"), Action.Name) || !Experience || !Experience->TryGetStringField(TEXT("experienceId"), Action.Experience.ExperienceId)) return;
    Action.Experience.bHasJourneyId = Experience->TryGetStringField(TEXT("journeyId"), Action.Experience.JourneyId);
    Action.Experience.bHasExperienceVersion = Experience->TryGetStringField(TEXT("experienceVersion"), Action.Experience.ExperienceVersion);
    auto Data = Child(Payload, TEXT("payload")); FString Error;
    Action.bHasPayload = Data.IsValid();
    if (Data && !NuxieWire::Scalars(Data, Action.Payload)) { Owner->OnError.Broadcast(InvalidResponse()); return; }
    Owner->OnAppAction.Broadcast(Action);
  } else if ((Name == TEXT("purchase") || Name == TEXT("restore")) && Owner->PurchaseController && !bChangingIdentity) {
    FString Id; double Deadline;
    if (!Payload->TryGetStringField(TEXT("requestId"), Id) || Id.IsEmpty() || !Payload->TryGetNumberField(TEXT("deadlineMs"), Deadline) || !FMath::IsFinite(Deadline)) return;
    if (Name == TEXT("purchase")) {
      if (!Child(Payload, TEXT("product"))) return;
      for (const auto& Existing : Owner->Purchases) if (Existing && Existing->RequestId == Id) return;
      auto Request = NewObject<UNuxiePurchaseRequest>(Owner.Get());
      Request->RequestId = Id; Request->DeadlineMs = Deadline; Request->Session = AsShared(); if (!NuxieWire::Product(Child(Payload, TEXT("product")), Request->Product)) { Owner->OnError.Broadcast(InvalidResponse()); return; }
      Owner->Purchases.Add(Request);
      if (Request->IsPending()) INuxiePurchaseController::Execute_BeginPurchase(Owner->PurchaseController, Request);
    } else {
      for (const auto& Existing : Owner->Restores) if (Existing && Existing->RequestId == Id) return;
      auto Request = NewObject<UNuxieRestoreRequest>(Owner.Get());
      Request->RequestId = Id; Request->DeadlineMs = Deadline; Request->Session = AsShared();
      Owner->Restores.Add(Request);
      if (Request->IsPending()) INuxiePurchaseController::Execute_BeginRestore(Owner->PurchaseController, Request);
    }
  }
}

#if WITH_DEV_AUTOMATION_TESTS
void FNuxieSession::SetTestTransport(TFunction<TUniquePtr<INuxieNativeTransport>()> Factory) {
  check(!NativeOwner); TestTransport = MoveTemp(Factory);
}
void FNuxieSession::AdvanceTestClock(double Seconds) { check(TestTransport); if (NativeOwner) NativeOwner->ActiveTime += Seconds; }
#endif

#include "NuxieAsyncActions.h"
#include "NuxieSession.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
namespace {
UNuxieSubsystem* Resolve(UObject* Context) {
  if (auto Instance = Cast<UGameInstance>(Context)) return Instance->GetSubsystem<UNuxieSubsystem>();
  if (auto Subsystem = Cast<UNuxieSubsystem>(Context)) return Subsystem;
  UWorld* World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull);
  return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UNuxieSubsystem>() : nullptr;
}
}

UNuxieConfigureAsyncAction* UNuxieConfigureAsyncAction::Configure(UObject* WorldContextObject, const FNuxieOptions& Options) {
  auto* Action = NewObject<UNuxieConfigureAsyncAction>();
  Action->Context = WorldContextObject;
  Action->Options = Options;
  return Action;
}
void UNuxieConfigureAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieConfigureAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->Configure(Options, FNuxieCompletion::CreateLambda([Weak](const FNuxieResult& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast();
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieShutdownAsyncAction* UNuxieShutdownAsyncAction::Shutdown(UObject* WorldContextObject) {
  auto* Action = NewObject<UNuxieShutdownAsyncAction>();
  Action->Context = WorldContextObject;
  return Action;
}
void UNuxieShutdownAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieShutdownAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->Shutdown(FNuxieCompletion::CreateLambda([Weak](const FNuxieResult& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast();
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieIdentifyAsyncAction* UNuxieIdentifyAsyncAction::Identify(UObject* WorldContextObject, const FString& CustomerId, const FNuxieIdentityOptions& Options) {
  auto* Action = NewObject<UNuxieIdentifyAsyncAction>();
  Action->Context = WorldContextObject;
  Action->CustomerId = CustomerId;
  Action->Options = Options;
  return Action;
}
void UNuxieIdentifyAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieIdentifyAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->Identify(CustomerId, Options, FNuxieCompletion::CreateLambda([Weak](const FNuxieResult& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast();
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieResetAsyncAction* UNuxieResetAsyncAction::Reset(UObject* WorldContextObject) {
  auto* Action = NewObject<UNuxieResetAsyncAction>();
  Action->Context = WorldContextObject;
  return Action;
}
void UNuxieResetAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieResetAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->Reset(FNuxieCompletion::CreateLambda([Weak](const FNuxieResult& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast();
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieGetIdentityAsyncAction* UNuxieGetIdentityAsyncAction::GetIdentity(UObject* WorldContextObject) {
  auto* Action = NewObject<UNuxieGetIdentityAsyncAction>();
  Action->Context = WorldContextObject;
  return Action;
}
void UNuxieGetIdentityAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieGetIdentityAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->GetIdentity(FNuxieIdentityCompletion::CreateLambda([Weak](const TNuxieResult<FNuxieIdentity>& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast(Result.GetValue());
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieCheckFeatureAsyncAction* UNuxieCheckFeatureAsyncAction::CheckFeature(UObject* WorldContextObject, const FString& FeatureId, const FNuxieFeatureQuery& Query) {
  auto* Action = NewObject<UNuxieCheckFeatureAsyncAction>();
  Action->Context = WorldContextObject;
  Action->FeatureId = FeatureId;
  Action->Query = Query;
  return Action;
}
void UNuxieCheckFeatureAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieCheckFeatureAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->CheckFeature(FeatureId, Query, FNuxieFeatureCompletion::CreateLambda([Weak](const TNuxieResult<FNuxieFeatureAccess>& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast(Result.GetValue());
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieConsumeFeatureAsyncAction* UNuxieConsumeFeatureAsyncAction::ConsumeFeature(UObject* WorldContextObject, const FString& FeatureId, const FNuxieFeatureCommand& Command) {
  auto* Action = NewObject<UNuxieConsumeFeatureAsyncAction>();
  Action->Context = WorldContextObject;
  Action->FeatureId = FeatureId;
  Action->Command = Command;
  return Action;
}
void UNuxieConsumeFeatureAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieConsumeFeatureAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->ConsumeFeature(FeatureId, Command, FNuxieConsumeCompletion::CreateLambda([Weak](const TNuxieResult<FNuxieUsageReceipt>& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast(Result.GetValue());
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieTriggerAsyncAction* UNuxieTriggerAsyncAction::Trigger(UObject* WorldContextObject, const FString& EventName, const FNuxieProperties& Properties) {
  auto* Action = NewObject<UNuxieTriggerAsyncAction>();
  Action->Context = WorldContextObject;
  Action->EventName = EventName;
  Action->Properties = Properties;
  return Action;
}
void UNuxieTriggerAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieTriggerAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->Trigger(EventName, Properties, FNuxieCompletion::CreateLambda([Weak](const FNuxieResult& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast();
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieDismissAsyncAction* UNuxieDismissAsyncAction::Dismiss(UObject* WorldContextObject) {
  auto* Action = NewObject<UNuxieDismissAsyncAction>();
  Action->Context = WorldContextObject;
  return Action;
}
void UNuxieDismissAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieDismissAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->Dismiss(FNuxieCompletion::CreateLambda([Weak](const FNuxieResult& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast();
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

UNuxieSetLocaleAsyncAction* UNuxieSetLocaleAsyncAction::SetLocale(UObject* WorldContextObject, const FString& Locale) {
  auto* Action = NewObject<UNuxieSetLocaleAsyncAction>();
  Action->Context = WorldContextObject;
  Action->Locale = Locale;
  return Action;
}
void UNuxieSetLocaleAsyncAction::Activate() {
  if (bActivated) return;
  bActivated = true;
  auto* Client = Resolve(Context);
  TWeakObjectPtr<UNuxieSetLocaleAsyncAction> Weak(this);
  if (!Client) {
    FNuxieSession::Defer([KeepAlive = TStrongObjectPtr<UBlueprintAsyncActionBase>(this), Weak]() { if (Weak.IsValid()) { Weak->Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A valid game instance is required."))); Weak->SetReadyToDestroy(); } });
    return;
  }
  RegisterWithGameInstance(Client->GetGameInstance());
  Client->SetLocale(Locale, FNuxieCompletion::CreateLambda([Weak](const FNuxieResult& Result) {
    if (!Weak.IsValid()) return;
    if (Result.IsSuccess()) Weak->Success.Broadcast();
    else Weak->Failure.Broadcast(Result.GetError());
    Weak->SetReadyToDestroy();
  }));
}

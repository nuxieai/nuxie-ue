#pragma once
#include "NuxieSubsystem.h"
#include "NuxieNativeTransport.h"
#include "NuxieWire.h"
#include "Core/RequestLedger.h"
#include "Containers/Ticker.h"

class FNuxieSession : public TSharedFromThis<FNuxieSession> {
public:
  using FReply = TFunction<void(NuxieWire::FObject, FNuxieError)>;
  explicit FNuxieSession(UNuxieSubsystem* Owner);
#if WITH_DEV_AUTOMATION_TESTS
  static void SetTestTransport(TFunction<TUniquePtr<INuxieNativeTransport>()> Factory);
  static void AdvanceTestClock(double Seconds);
#endif
  ~FNuxieSession();
  static void Configure(UNuxieSubsystem* Owner, const FNuxieOptions& Options, FNuxieCompletion Completion);
  static void Defer(TFunction<void()> Callback);
  static bool HasDeferredCallbacks();
  static bool HasNativeOwner();
  static void ShutdownFor(UNuxieSubsystem* Owner, FNuxieCompletion Completion);
  void Shutdown(FNuxieCompletion Completion);
  void ChangeIdentity(const FString& Method, NuxieWire::FObject Arguments, FNuxieCompletion Completion);
  void Call(const FString& Method, NuxieWire::FObject Arguments, bool bDurable, FReply Reply);
  bool CompleteCheckout(const FString& Method, const FString& RequestId, const FString& Result);
  FNuxieStatus Status;
  FNuxieFeatureSnapshot Features;
  FNuxieIdentity Identity;
private:
  TWeakObjectPtr<UNuxieSubsystem> Owner;
  TUniquePtr<INuxieNativeTransport> Transport;
  FString SessionId;
  FString Configuration;
  TWeakObjectPtr<UObject> Controller;
  uint64 IdentityEpoch = 0;
  bool bChangingIdentity = false;
  bool bForeground = true;
  double ActiveTime = 0;
  nuxie::RequestLedger Ledger;
  TMap<FString, FReply> Replies;
  TArray<FNuxieCompletion> ConfigureWaiters;
  TArray<FNuxieCompletion> ShutdownWaiters;
  FTSTicker::FDelegateHandle Ticker;
  FDelegateHandle BackgroundHandle;
  FDelegateHandle ForegroundHandle;
  void Start();
  bool Tick(float Delta);
  bool Send(const FString& Method, NuxieWire::FObject Arguments, bool bDurable, FReply Reply);
  void Finish(const FString& Id, NuxieWire::FObject Value, FNuxieError Error);
  void SetStatus(ENuxieStatusKind Kind, FNuxieError Error = FNuxieError());
  bool AdoptIdentity(const NuxieWire::FObject& Value);
  void Event(const NuxieWire::FObject& Envelope);
  void PublishFeatures();
};

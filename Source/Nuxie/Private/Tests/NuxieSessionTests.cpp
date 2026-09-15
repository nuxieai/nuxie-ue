#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "NuxieSession.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

namespace {
using NuxieWire::FObject;
struct FNativeFixture {
  TArray<FString> Messages;
  FString Customer = TEXT("anonymous");
  int32 Generation = 1;
  int32 Configures = 0;
  int32 Shutdowns = 0;
  FString HeldQuery;
  FString HeldConsume;
  FString ConsumingCustomer;
  FString Session;
  FObject Object() { return MakeShared<FJsonObject>(); }
  FObject Identity() { auto V = Object(); V->SetStringField(TEXT("distinctId"), Customer); V->SetStringField(TEXT("anonymousId"), TEXT("anonymous")); V->SetBoolField(TEXT("isIdentified"), Customer == TEXT("alice")); return V; }
  FObject Snapshot() { auto V = Object(); V->SetStringField(TEXT("identityGeneration"), FString::FromInt(Generation)); V->SetStringField(TEXT("revision"), TEXT("1")); V->SetStringField(TEXT("state"), TEXT("ready")); V->SetObjectField(TEXT("all"), Object()); return V; }
  void Reply(const FString& Id, FObject Value) { auto V = Object(); V->SetStringField(TEXT("requestId"), Id); if (Value) V->SetStringField(TEXT("result"), NuxieWire::Json(Value)); else V->SetField(TEXT("result"), MakeShared<FJsonValueNull>()); Messages.Add(NuxieWire::Json(V)); }
  void Accept(const FString& Json) {
    auto V = NuxieWire::Object(Json); const FString Id = V->GetStringField(TEXT("requestId")); const FString Method = V->GetStringField(TEXT("method")); auto Args = V->GetObjectField(TEXT("arguments"));
    if (Method == TEXT("configure")) {
      ++Configures; Session = NuxieWire::Object(Args->GetStringField(TEXT("configuration")))->GetStringField(TEXT("session"));
      auto R = Object(); R->SetNumberField(TEXT("contract"), 1); R->SetStringField(TEXT("session"), Session); R->SetObjectField(TEXT("identity"), Identity()); R->SetObjectField(TEXT("snapshot"), Snapshot()); Reply(Id, R);
    } else if (Method == TEXT("identify") || Method == TEXT("reset")) {
      ++Generation; Customer = Method == TEXT("identify") ? Args->GetStringField(TEXT("customerId")) : TEXT("rotated");
      auto R = Object(); R->SetObjectField(TEXT("identity"), Identity()); R->SetObjectField(TEXT("snapshot"), Snapshot()); Reply(Id, R);
    } else if (Method == TEXT("hasFeature")) HeldQuery = Id;
    else if (Method == TEXT("consumeFeature")) { HeldConsume = Id; ConsumingCustomer = Customer; }
    else if (Method == TEXT("getIdentity")) Reply(Id, Identity());
    else { if (Method == TEXT("shutdown")) ++Shutdowns; Reply(Id, nullptr); }
  }
  void ReleaseQuery() { Reply(HeldQuery, NuxieWire::Object(TEXT("{\"allowed\":true,\"unlimited\":false,\"balance\":2,\"type\":\"metered\"}"))); }
  void ReleaseConsume() { auto R = NuxieWire::Object(TEXT("{\"featureId\":\"energy\",\"operationId\":\"saved\",\"quantity\":1,\"occurredAtMs\":null,\"accepted\":true,\"code\":\"accepted\",\"balance\":0,\"unlimited\":false,\"active\":true,\"idempotentReplay\":false}")); R->SetStringField(TEXT("customerId"), ConsumingCustomer); Reply(HeldConsume, R); }
};
class FFixtureTransport : public INuxieNativeTransport {
  TSharedRef<FNativeFixture> Fixture;
public:
  explicit FFixtureTransport(TSharedRef<FNativeFixture> InFixture) : Fixture(InFixture) {}
  int32 ContractVersion() override { return 1; }
  bool Submit(const FString& Request) override { Fixture->Accept(Request); return true; }
  bool Poll(FString& Message) override { if (Fixture->Messages.IsEmpty()) return false; Message = Fixture->Messages[0]; Fixture->Messages.RemoveAt(0); return true; }
};
class FLifecycleCommand : public IAutomationLatentCommand {
  FAutomationTestBase* Test;
  TSharedRef<FNativeFixture> Fixture = MakeShared<FNativeFixture>();
  TStrongObjectPtr<UGameInstance> InstanceA{NewObject<UGameInstance>()};
  TStrongObjectPtr<UGameInstance> InstanceB{NewObject<UGameInstance>()};
  TStrongObjectPtr<UNuxieSubsystem> A{NewObject<UNuxieSubsystem>(InstanceA.Get())};
  TStrongObjectPtr<UNuxieSubsystem> B{NewObject<UNuxieSubsystem>(InstanceB.Get())};
  FNuxieOptions Options;
  int32 Phase = 0, SetupReplies = 0, BurstReplies = 0, BurstRejected = 0;
  bool bIdentityOverloaded = false;
  bool bOtherRejected = false, bIdentified = false, bQueryFinished = false, bReset = false, bConsumed = false, bTimedOut = false, bShutdown = false, bReconfigured = false;
  double Started = FPlatformTime::Seconds();
public:
  explicit FLifecycleCommand(FAutomationTestBase* InTest) : Test(InTest) { Options.AndroidPublicKey = TEXT("public_test"); Options.IOSPublicKey = TEXT("public_test"); }
  bool Update() override {
    if (Phase == 8) { if (bShutdown) { FNuxieSession::SetTestTransport({}); return true; } return false; }
    if (FPlatformTime::Seconds() - Started > 60) { Test->AddError(TEXT("Lifecycle test timed out.")); A->Shutdown(FNuxieCompletion::CreateLambda([this](const FNuxieResult&) { bShutdown = true; })); Phase = 8; return false; }
    if (Phase == 0) {
      FNuxieSession::SetTestTransport([State = Fixture]() { return MakeUnique<FFixtureTransport>(State); });
      auto Done = FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) { Test->TestTrue(TEXT("configure succeeds"), R.IsSuccess()); ++SetupReplies; });
      A->Configure(Options, Done); A->Configure(Options, Done);
      B->Configure(Options, FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) { Test->TestTrue(TEXT("other game instance cannot share native identity"), R.GetError().Code == ENuxieErrorCode::SessionInUse); bOtherRejected = true; }));
      Phase = 1;
    } else if (Phase == 1 && SetupReplies == 2 && bOtherRejected) {
      Test->TestEqual(TEXT("equivalent setup calls share native setup"), Fixture->Configures, 1);
      A->CheckFeature(TEXT("energy"), FNuxieFeatureQuery(), FNuxieFeatureCompletion::CreateLambda([this](const TNuxieResult<FNuxieFeatureAccess>& R) { Test->TestTrue(TEXT("old query is fenced"), !R.IsSuccess() && R.GetError().Code == ENuxieErrorCode::IdentityChanged); bQueryFinished = true; }));
      A->Identify(TEXT("alice"), FNuxieIdentityOptions(), FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) { Test->TestTrue(TEXT("identify succeeds"), R.IsSuccess()); bIdentified = true; })); Phase = 2;
    } else if (Phase == 2 && bIdentified) { Fixture->ReleaseQuery(); Phase = 3; }
    else if (Phase == 3 && bQueryFinished) {
      Test->TestEqual(TEXT("snapshot customer adopted atomically"), A->GetFeatureSnapshot().CustomerId, FString(TEXT("alice")));
      FNuxieFeatureCommand Command; Command.OperationId = TEXT("saved");
      A->ConsumeFeature(TEXT("energy"), Command, FNuxieConsumeCompletion::CreateLambda([this](const TNuxieResult<FNuxieUsageReceipt>& R) { Test->TestTrue(TEXT("admitted original-customer receipt survives reset"), R.IsSuccess()); if (R.IsSuccess()) { Test->TestEqual(TEXT("receipt owner is original"), R.GetValue().CustomerId, FString(TEXT("alice"))); Test->TestTrue(TEXT("last unit accepted"), R.GetValue().bAccepted); } bConsumed = true; }));
      A->Reset(FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) { Test->TestTrue(TEXT("reset succeeds"), R.IsSuccess()); bReset = true; })); Phase = 4;
    } else if (Phase == 4 && bReset) { Fixture->ReleaseConsume(); Phase = 5; }
    else if (Phase == 5 && bConsumed) {
      Test->TestEqual(TEXT("receipt does not replace new customer state"), A->GetFeatureSnapshot().CustomerId, FString(TEXT("rotated")));
      A->CheckFeature(TEXT("energy"), FNuxieFeatureQuery(), FNuxieFeatureCompletion::CreateLambda([this](const TNuxieResult<FNuxieFeatureAccess>& R) { Test->TestTrue(TEXT("missing native reply times out"), !R.IsSuccess() && R.GetError().Code == ENuxieErrorCode::OperationTimeout); bTimedOut = true; }));
      FNuxieSession::AdvanceTestClock(91); Phase = 6;
    } else if (Phase == 6 && bTimedOut) { A->Shutdown(FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) { Test->TestTrue(TEXT("shutdown acknowledged"), R.IsSuccess()); bShutdown = true; })); Phase = 7; }
    else if (Phase == 7 && bShutdown) {
      Test->TestTrue(TEXT("shutdown clears state"), A->GetStatus().Kind == ENuxieStatusKind::Unconfigured);
      bShutdown = false;
      A->Configure(Options, FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) {
        Test->TestTrue(TEXT("shutdown cancels in-flight configuration"), R.GetError().Code == ENuxieErrorCode::SDKShutdown);
        Test->TestTrue(TEXT("cancelled setup cannot overwrite ShuttingDown with Failed"), A->GetStatus().Kind == ENuxieStatusKind::ShuttingDown);
        A->Shutdown(FNuxieCompletion());
      }));
      A->Shutdown(FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) { Test->TestTrue(TEXT("reentrant shutdown succeeds"), R.IsSuccess()); bShutdown = true; }));
      Phase = 9;
    } else if (Phase == 9 && bShutdown) {
      Test->TestEqual(TEXT("reentrant cancellation sends one native shutdown per session"), Fixture->Shutdowns, 2);
      bShutdown = false;
      A->Configure(Options, FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) {
        Test->TestTrue(TEXT("setup before callback reconfiguration succeeds"), R.IsSuccess());
        A->Shutdown(FNuxieCompletion::CreateLambda([this](const FNuxieResult& Closed) {
          Test->TestTrue(TEXT("shutdown callback may reconfigure"), Closed.IsSuccess());
          A->Configure(Options, FNuxieCompletion::CreateLambda([this](const FNuxieResult& Reopened) {
            Test->TestTrue(TEXT("old poller cannot steal new session reply"), Reopened.IsSuccess()); bReconfigured = true;
          }));
        }));
      }));
      Phase = 10;
    } else if (Phase == 10 && bReconfigured) {
      for (int32 Index = 0; Index < 65; ++Index) {
        A->GetIdentity(FNuxieIdentityCompletion::CreateLambda([this](const TNuxieResult<FNuxieIdentity>& R) {
          if (R.IsSuccess()) ++BurstReplies;
          else { Test->TestTrue(TEXT("excess admission fails explicitly"), R.GetError().Code == ENuxieErrorCode::Overloaded); ++BurstRejected; }
        }));
      }
      A->Identify(TEXT("unadmitted"), FNuxieIdentityOptions(), FNuxieCompletion::CreateLambda([this](const FNuxieResult& R) {
        Test->TestTrue(TEXT("identity saturation rejects without changing customer"), R.GetError().Code == ENuxieErrorCode::Overloaded);
        Test->TestTrue(TEXT("rejected identity preserves ready session"), A->GetStatus().Kind == ENuxieStatusKind::Ready);
        Test->TestEqual(TEXT("rejected identity preserves snapshot customer"), A->GetFeatureSnapshot().CustomerId, FString(TEXT("rotated")));
        bIdentityOverloaded = true;
      }));
      Phase = 11;
    } else if (Phase == 11 && BurstReplies + BurstRejected == 65 && bIdentityOverloaded) {
      Test->TestEqual(TEXT("reserved admission settles every accepted operation"), BurstReplies, 64);
      Test->TestEqual(TEXT("excess operation rejected"), BurstRejected, 1);
      A->Shutdown(FNuxieCompletion::CreateLambda([this](const FNuxieResult&) { bShutdown = true; })); Phase = 8;
    }
    return false;
  }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNuxieSessionLifecycle, "Nuxie.Contract.SessionLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNuxieSessionLifecycle::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(FLifecycleCommand(this)); return true; }
namespace {
class FDeferredBudgetCommand : public IAutomationLatentCommand {
  FAutomationTestBase* Test;
  TSharedPtr<TMap<uint64, int32>> PerFrame = MakeShared<TMap<uint64, int32>>();
  TSharedPtr<int32> Delivered = MakeShared<int32>(0);
  bool Started = false;
  double Deadline = 0;
public:
  explicit FDeferredBudgetCommand(FAutomationTestBase* InTest) : Test(InTest) {}
  bool Update() override {
    if (!Started) {
      Started = true; Deadline = FPlatformTime::Seconds() + 10;
      for (int32 Index = 0; Index < 600; ++Index) {
        FNuxieSession::Defer([Frames = PerFrame, Count = Delivered]() {
          ++Frames->FindOrAdd(GFrameCounter); ++*Count;
          // Exercise work scheduled during delivery, as an App Action handler does.
          FNuxieSession::Defer([Frames, Count]() { ++Frames->FindOrAdd(GFrameCounter); ++*Count; });
        });
      }
      Test->TestEqual(TEXT("completion remains deferred"), *Delivered, 0);
      return false;
    }
    if (*Delivered < 1200 && FPlatformTime::Seconds() < Deadline) return false;
    Test->TestEqual(TEXT("all initial and nested callbacks settle"), *Delivered, 1200);
    for (const auto& Entry : *PerFrame) Test->TestTrue(TEXT("deferred work yields after 256 callbacks in one frame"), Entry.Value <= 256);
    Test->TestTrue(TEXT("burst spans multiple engine frames"), PerFrame->Num() >= 5);
    return true;
  }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNuxieDeferredBudget, "Nuxie.Contract.DeferredBudget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNuxieDeferredBudget::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(FDeferredBudgetCommand(this)); return true; }
#endif

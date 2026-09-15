#include "NuxiePurchaseController.h"
#include "NuxieSession.h"
#include "Misc/DateTime.h"
namespace {
double NowMs() { return static_cast<double>(FDateTime::UtcNow().GetTicks() - FDateTime(1970, 1, 1).GetTicks()) / ETimespan::TicksPerMillisecond; }
FString OutcomeJson(const FString& Type, const FString& Message) { auto Value = MakeShared<FJsonObject>(); Value->SetStringField(TEXT("type"), Type); Value->SetStringField(TEXT("message"), Message); return NuxieWire::Json(Value); }
}
bool UNuxiePurchaseRequest::IsPendingAt(double Now) const { return bPending && Session.IsValid() && Now < DeadlineMs; }
bool UNuxieRestoreRequest::IsPendingAt(double Now) const { return bPending && Session.IsValid() && Now < DeadlineMs; }
bool UNuxiePurchaseRequest::IsPending() const { return IsPendingAt(NowMs()); }
bool UNuxieRestoreRequest::IsPending() const { return IsPendingAt(NowMs()); }
bool UNuxiePurchaseRequest::TryComplete(ENuxiePurchaseOutcome Outcome, const FString& Message) {
  check(IsInGameThread());
  switch (Outcome) {
    case ENuxiePurchaseOutcome::Purchased:
    case ENuxiePurchaseOutcome::Cancelled:
    case ENuxiePurchaseOutcome::Pending:
    case ENuxiePurchaseOutcome::Failed: break;
    default: return false;
  }
  if (!IsPending()) return false;
  const bool bAccepted = Session.Pin()->CompleteCheckout(TEXT("completePurchase"), RequestId, OutcomeJson(StaticEnum<ENuxiePurchaseOutcome>()->GetNameStringByValue(static_cast<int64>(Outcome)).ToLower(), Message));
  if (bAccepted) bPending = false;
  return bAccepted;
}
bool UNuxieRestoreRequest::TryComplete(ENuxieRestoreOutcome Outcome, const FString& Message) {
  check(IsInGameThread());
  switch (Outcome) {
    case ENuxieRestoreOutcome::Restored:
    case ENuxieRestoreOutcome::NoPurchases:
    case ENuxieRestoreOutcome::Failed: break;
    default: return false;
  }
  if (!IsPending()) return false;
  const FString Type = Outcome == ENuxieRestoreOutcome::NoPurchases ? TEXT("noPurchases") : StaticEnum<ENuxieRestoreOutcome>()->GetNameStringByValue(static_cast<int64>(Outcome)).ToLower();
  const bool bAccepted = Session.Pin()->CompleteCheckout(TEXT("completeRestore"), RequestId, OutcomeJson(Type, Message));
  if (bAccepted) bPending = false;
  return bAccepted;
}
void INuxiePurchaseController::BeginPurchase_Implementation(UNuxiePurchaseRequest* Request) { Request->TryComplete(ENuxiePurchaseOutcome::Failed, TEXT("BeginPurchase is not implemented.")); }
void INuxiePurchaseController::BeginRestore_Implementation(UNuxieRestoreRequest* Request) { Request->TryComplete(ENuxieRestoreOutcome::Failed, TEXT("BeginRestore is not implemented.")); }

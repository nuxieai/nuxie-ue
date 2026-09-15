#include "NuxieWire.h"
#include "Core/RequestLedger.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace NuxieWire {
FString JsonValue(const TSharedPtr<FJsonValue>& Value) {
  if (!Value) return FString();
  FString Json;
  TArray<TSharedPtr<FJsonValue>> Values = { Value };
  if (!FJsonSerializer::Serialize(Values, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json))) return FString();
  return Json.Mid(1, Json.Len() - 2);
}
TSharedPtr<FJsonValue> Value(const FString& Text) {
  // Unreal deserializes containers only. A one-element array admits scalar roots too.
  TArray<TSharedPtr<FJsonValue>> Values;
  if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(TEXT("[") + Text + TEXT("]")), Values) || Values.Num() != 1) return nullptr;
  return Values[0];
}
FObject Object(const FString& Text) { FObject R; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R); return R; }
FString Json(const FObject& Value) { FString R; if (Value) FJsonSerializer::Serialize(Value.ToSharedRef(), TJsonWriterFactory<>::Create(&R)); return R; }
FNuxieError Error(ENuxieErrorCode Code, const FString& Message, const FString& NativeCode) {
  FNuxieError R; R.Code=Code; R.Message=Message; R.NativeCode=NativeCode; return R;
}
FNuxieError NativeError(const FObject& Value) {
  FString Code,Message;
  if (!Value || !Value->TryGetStringField(TEXT("code"),Code) || !Value->TryGetStringField(TEXT("message"),Message)) return Error(ENuxieErrorCode::InvalidResponse,TEXT("Malformed native error"));
  ENuxieErrorCode Kind=ENuxieErrorCode::NativeError;
  if (Code==TEXT("invalidArgument")) Kind=ENuxieErrorCode::InvalidArgument;
  else if (Code==TEXT("incompatibleBridge")) Kind=ENuxieErrorCode::IncompatibleBridge;
  else if (Code==TEXT("alreadyConfigured")) Kind=ENuxieErrorCode::AlreadyConfigured;
  else if (Code==TEXT("engineAlreadyAttached")) Kind=ENuxieErrorCode::SessionInUse;
  else if (Code==TEXT("sessionExpired")) Kind=ENuxieErrorCode::SDKShutdown;
  return Error(Kind,Message,Code);
}
static bool NullableNumber(const FObject& V,const TCHAR* Key,bool& Present,double& Number) {
  auto Found=V->Values.Find(Key); if(!Found) return false;
  Present=(*Found)->Type!=EJson::Null;
  return !Present || ((*Found)->TryGetNumber(Number) && FMath::IsFinite(Number));
}
static bool Integer(const FObject& V,const TCHAR* Key,int64& Out,bool Positive=false) {
  double N; if(!V->TryGetNumberField(Key,N)||!FMath::IsFinite(N)||N!=FMath::FloorToDouble(N)||FMath::Abs(N)>9007199254740991.0||(Positive&&N<=0))return false;
  Out=static_cast<int64>(N); return true;
}
bool Access(const FObject& V,FNuxieFeatureAccess& Out) {
  FString Type;
  if(!V||!V->TryGetBoolField(TEXT("allowed"),Out.bAllowed)||!V->TryGetBoolField(TEXT("unlimited"),Out.bUnlimited)||!NullableNumber(V,TEXT("balance"),Out.bHasBalance,Out.Balance)||!V->TryGetStringField(TEXT("type"),Type))return false;
  if(Type==TEXT("boolean"))Out.Type=ENuxieFeatureType::Boolean;
  else if(Type==TEXT("metered"))Out.Type=ENuxieFeatureType::Metered;
  else if(Type==TEXT("creditSystem"))Out.Type=ENuxieFeatureType::CreditSystem;
  else return false;
  return true;
}
bool Identity(const FObject& V,FNuxieIdentity& Out) {
  return V&&V->TryGetStringField(TEXT("distinctId"),Out.CustomerId)&&!Out.CustomerId.IsEmpty()&&V->TryGetStringField(TEXT("anonymousId"),Out.AnonymousId)&&V->TryGetBoolField(TEXT("isIdentified"),Out.bIdentified);
}
bool Receipt(const FObject& V,FNuxieUsageReceipt& Out) {
  double Time=0;
  if(!V||!V->TryGetStringField(TEXT("customerId"),Out.CustomerId)||Out.CustomerId.IsEmpty()||!V->TryGetStringField(TEXT("featureId"),Out.FeatureId)||!V->TryGetStringField(TEXT("operationId"),Out.OperationId)||!Integer(V,TEXT("quantity"),Out.Quantity,true)||!V->TryGetBoolField(TEXT("accepted"),Out.bAccepted)||!V->TryGetStringField(TEXT("code"),Out.Code)||!NullableNumber(V,TEXT("balance"),Out.bHasBalance,Out.Balance)||!NullableNumber(V,TEXT("occurredAtMs"),Out.bHasOccurredAt,Time)||!V->TryGetBoolField(TEXT("unlimited"),Out.bUnlimited)||!V->TryGetBoolField(TEXT("active"),Out.bActive)||!V->TryGetBoolField(TEXT("idempotentReplay"),Out.bIdempotentReplay))return false;
  return !Out.bHasOccurredAt || Integer(V,TEXT("occurredAtMs"),Out.OccurredAtMs);
}
bool Snapshot(const FObject& V,FNuxieFeatureSnapshot& Out) {
  FString State; const FObject* All=nullptr;
  if(!V||!V->TryGetStringField(TEXT("identityGeneration"),Out.IdentityGeneration)||!nuxie::counter(TCHAR_TO_UTF8(*Out.IdentityGeneration))||!V->TryGetStringField(TEXT("revision"),Out.Revision)||!nuxie::counter(TCHAR_TO_UTF8(*Out.Revision))||!V->TryGetStringField(TEXT("state"),State)||!V->TryGetObjectField(TEXT("all"),All))return false;
  if(State==TEXT("ready"))Out.Kind=ENuxieFeatureStateKind::Ready;
  else if(State==TEXT("unknown"))Out.Kind=ENuxieFeatureStateKind::Unknown;
  else if(State==TEXT("reconciling"))Out.Kind=ENuxieFeatureStateKind::Reconciling;
  else return false;
  Out.All.Empty();
  for(const auto& Pair:(*All)->Values) { FNuxieFeatureAccess A; const FObject* Obj=nullptr; if(!Pair.Value->TryGetObject(Obj)||!Access(*Obj,A))return false; Out.All.Add(FString(Pair.Key.ToView()),A); }
  return true;
}
FNuxieProperties Properties(const FObject& V) { FNuxieProperties P; FString E; FNuxieProperties::TryParse(V?Json(V):TEXT("{}"),P,E);return P; }
bool Scalars(const FObject& Value, TMap<FString, FNuxieScalar>& Out) {
  if (!Value) return false;
  TMap<FString, FNuxieScalar> Result;
  for (const auto& Pair : Value->Values) {
    const FObject* Object = nullptr; FString Kind; FNuxieScalar Scalar;
    if (!Pair.Value->TryGetObject(Object) || !(*Object)->TryGetStringField(TEXT("kind"), Kind)) return false;
    if (Kind == TEXT("string")) { Scalar.Kind = ENuxieScalarKind::String; if (!(*Object)->TryGetStringField(TEXT("value"), Scalar.String)) return false; }
    else if (Kind == TEXT("integer")) {
      Scalar.Kind = ENuxieScalarKind::Integer; FString Digits;
      if (!(*Object)->TryGetStringField(TEXT("value"), Digits)) return false;
      const bool Negative = Digits.StartsWith(TEXT("-"));
      auto Magnitude = nuxie::counter(TCHAR_TO_UTF8(*(Negative ? Digits.Mid(1) : Digits)));
      if (!Magnitude || *Magnitude > (Negative ? 9223372036854775808ULL : 9223372036854775807ULL)) return false;
      Scalar.Integer = Negative ? (*Magnitude == 9223372036854775808ULL ? MIN_int64 : -static_cast<int64>(*Magnitude)) : static_cast<int64>(*Magnitude);
    } else if (Kind == TEXT("number")) { Scalar.Kind = ENuxieScalarKind::Number; if (!(*Object)->TryGetNumberField(TEXT("value"), Scalar.Number) || !FMath::IsFinite(Scalar.Number)) return false; }
    else if (Kind == TEXT("boolean")) { Scalar.Kind = ENuxieScalarKind::Boolean; if (!(*Object)->TryGetBoolField(TEXT("value"), Scalar.Boolean)) return false; }
    else return false;
    Result.Add(FString(Pair.Key.ToView()), Scalar);
  }
  Out = MoveTemp(Result); return true;
}
static bool OptionalString(const FObject& Object, const TCHAR* Key, bool& Present, FString& String) {
  const auto* Value = Object->Values.Find(Key);
  Present = Value && (*Value)->Type != EJson::Null;
  return !Present || (*Value)->TryGetString(String);
}
bool Product(const FObject& V, FNuxieStoreProduct& P) {
  if (!V || !V->TryGetStringField(TEXT("platform"), P.Platform) || (P.Platform != TEXT("ios") && P.Platform != TEXT("android")) || !V->TryGetStringField(TEXT("productId"), P.ProductId) || P.ProductId.IsEmpty() || !V->TryGetStringField(TEXT("storeProductId"), P.StoreProductId) || P.StoreProductId.IsEmpty() || !V->TryGetStringField(TEXT("placementId"), P.PlacementId)) return false;
#define FIELD(Key, Field) if (!OptionalString(V, TEXT(Key), P.bHas##Field, P.Field)) return false
  FIELD("displayName", DisplayName); FIELD("description", Description); FIELD("displayPrice", DisplayPrice); FIELD("productType", ProductType);
  FIELD("period", Period); FIELD("billingPlan", BillingPlan); FIELD("eligibilityJws", EligibilityJws); FIELD("basePlanId", BasePlanId); FIELD("purchaseOptionId", PurchaseOptionId); FIELD("offerId", OfferId);
#undef FIELD
  const auto* Period = V->Values.Find(TEXT("periodCount"));
  P.bHasPeriodCount = Period && (*Period)->Type != EJson::Null;
  if (P.bHasPeriodCount && !Integer(V, TEXT("periodCount"), P.PeriodCount, true)) return false;
  const auto* RawTerms = V->Values.Find(TEXT("introductoryTerms"));
  P.bHasIntroductoryTerms = RawTerms && (*RawTerms)->Type != EJson::Null;
  if (P.bHasIntroductoryTerms) {
    const FObject* T = nullptr; auto& Terms = P.IntroductoryTerms;
    if (!(*RawTerms)->TryGetObject(T) || !(*T)->TryGetStringField(TEXT("price"), Terms.DisplayPrice) || !(*T)->TryGetStringField(TEXT("period"), Terms.Period) || !Integer(*T, TEXT("periodCount"), Terms.PeriodCount, true) || !Integer(*T, TEXT("cycles"), Terms.Cycles, true) || !(*T)->TryGetStringField(TEXT("paymentMode"), Terms.PaymentMode) || !(*T)->TryGetStringField(TEXT("displayDuration"), Terms.DisplayDuration)) return false;
  }
  const auto* RawPhases = V->Values.Find(TEXT("pricingPhases"));
  P.bHasPricingPhases = RawPhases && (*RawPhases)->Type != EJson::Null;
  P.PricingPhases.Reset();
  if (P.bHasPricingPhases) {
    const TArray<TSharedPtr<FJsonValue>>* Phases = nullptr;
    if (!(*RawPhases)->TryGetArray(Phases)) return false;
    for (const auto& Value : *Phases) {
      const FObject* O = nullptr; FNuxiePricingPhase Phase;
      if (!Value->TryGetObject(O) || !(*O)->TryGetStringField(TEXT("displayPrice"), Phase.DisplayPrice) || !(*O)->TryGetStringField(TEXT("billingPeriod"), Phase.BillingPeriod) || !Integer(*O, TEXT("billingCycleCount"), Phase.BillingCycleCount) || !Integer(*O, TEXT("recurrenceMode"), Phase.RecurrenceMode)) return false;
      P.PricingPhases.Add(Phase);
    }
  }
  return true;
}
}

bool NuxieWire::EventMatchesIdentity(const FObject& Value, const FString& Session, const FString& Generation, bool bChangingIdentity) {
  FString EventSession, EventGeneration;
  return Value && !bChangingIdentity && !Session.IsEmpty() && !Generation.IsEmpty()
    && Value->TryGetStringField(TEXT("session"), EventSession) && EventSession == Session
    && Value->TryGetStringField(TEXT("identityGeneration"), EventGeneration) && EventGeneration == Generation;
}

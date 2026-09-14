#pragma once
#include "CoreMinimal.h"
#include "NuxieTypes.generated.h"

UENUM(BlueprintType)
enum class ENuxieEnvironment : uint8 { Production, Development };

UENUM(BlueprintType)
enum class ENuxieLogLevel : uint8 { Warning, Debug, Info, Error, None, Verbose };

UENUM(BlueprintType)
enum class ENuxieBillingMode : uint8 { Native, External };

UENUM(BlueprintType)
enum class ENuxieStatusKind : uint8 { Unconfigured, Configuring, Ready, ShuttingDown, Failed };

UENUM(BlueprintType)
enum class ENuxieFeatureStateKind : uint8 { Unknown, Reconciling, Ready };

UENUM(BlueprintType)
enum class ENuxieFeatureType : uint8 { Boolean, Metered, CreditSystem };

UENUM(BlueprintType)
enum class ENuxieFeaturePolicy : uint8 { CacheFirst, Remote };

UENUM(BlueprintType)
enum class ENuxieErrorCode : uint8 { None, UnsupportedPlatform, InvalidArgument, NotConfigured, AlreadyConfigured, LifecycleBusy, IdentityChanged, SessionInUse, SDKShutdown, OperationTimeout, InvalidResponse, IncompatibleBridge, NativeError };

UENUM(BlueprintType)
enum class ENuxiePurchaseOutcome : uint8 { Purchased, Cancelled, Pending, Failed };

UENUM(BlueprintType)
enum class ENuxieRestoreOutcome : uint8 { Restored, NoPurchases, Failed };

/** JSON is stored by value. Use NuxieValues builders or validated parsing. */
USTRUCT(BlueprintType)
struct NUXIE_API FNuxieProperties
{
  GENERATED_BODY()
  FString ToJson() const { return Json; }
  static bool TryParse(const FString& Input, FNuxieProperties& Output, FString& Error);
private:
  UPROPERTY() FString Json = TEXT("{}");
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieJsonValue
{
  GENERATED_BODY()
  FString ToJson() const { return Json; }
  static bool TryParse(const FString& Input, FNuxieJsonValue& Output, FString& Error);
private:
  UPROPERTY() FString Json = TEXT("null");
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieError
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") ENuxieErrorCode Code = ENuxieErrorCode::None;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Message;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString NativeCode;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FNuxieProperties Details;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieStatus
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") ENuxieStatusKind Kind = ENuxieStatusKind::Unconfigured;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FNuxieError Error;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieOptions
{
  GENERATED_BODY()
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FString IOSPublicKey;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FString AndroidPublicKey;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") ENuxieEnvironment Environment = ENuxieEnvironment::Production;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") ENuxieLogLevel LogLevel = ENuxieLogLevel::Warning;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FString Locale;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") ENuxieBillingMode BillingMode = ENuxieBillingMode::Native;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") TObjectPtr<UObject> ExternalController = nullptr;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieIdentityOptions
{
  GENERATED_BODY()
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FNuxieProperties Properties;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FNuxieProperties PropertiesSetOnce;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieIdentity
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString CustomerId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString AnonymousId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bIdentified = false;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureAccess
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bAllowed = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bUnlimited = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasBalance = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") double Balance = 0.0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") ENuxieFeatureType Type = ENuxieFeatureType::Boolean;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureState
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") ENuxieFeatureStateKind Kind = ENuxieFeatureStateKind::Unknown;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString CustomerId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString IdentityGeneration = TEXT("0");
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Revision = TEXT("0");
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasAccess = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FNuxieFeatureAccess Access;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureSnapshot
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") ENuxieFeatureStateKind Kind = ENuxieFeatureStateKind::Unknown;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString CustomerId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString IdentityGeneration = TEXT("0");
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Revision = TEXT("0");
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") TMap<FString, FNuxieFeatureAccess> All;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureQuery
{
  GENERATED_BODY()
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FString EntityId;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") int64 RequiredBalance = 1;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") ENuxieFeaturePolicy Policy = ENuxieFeaturePolicy::CacheFirst;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureCommand
{
  GENERATED_BODY()
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FString OperationId;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") int64 Quantity = 1;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nuxie") FString EntityId;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieUsageReceipt
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString CustomerId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString FeatureId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString OperationId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 Quantity = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasOccurredAt = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 OccurredAtMs = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bAccepted = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Code;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasBalance = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") double Balance = 0.0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bUnlimited = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bActive = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bIdempotentReplay = false;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieExperienceContext
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString ExperienceId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasExperienceVersion = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString ExperienceVersion;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasJourneyId = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString JourneyId;
};

UENUM(BlueprintType)
enum class ENuxieScalarKind : uint8 { String, Integer, Number, Boolean };
USTRUCT(BlueprintType)
struct NUXIE_API FNuxieScalar {
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") ENuxieScalarKind Kind = ENuxieScalarKind::String;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString String;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 Integer = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") double Number = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool Boolean = false;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieActivity
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Id;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Name;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 TimestampMs = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 ReceivedAtMs = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") TMap<FString, FNuxieScalar> Properties;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieAppAction
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasPayload = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Name;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") TMap<FString, FNuxieScalar> Payload;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FNuxieExperienceContext Experience;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxiePricingPhase
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString DisplayPrice;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString BillingPeriod;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 BillingCycleCount = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 RecurrenceMode = 0;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieIntroductoryTerms {
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString DisplayPrice;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Period;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 PeriodCount = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 Cycles = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString PaymentMode;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString DisplayDuration;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieStoreProduct
{
  GENERATED_BODY()
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Platform;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString ProductId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString StoreProductId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString PlacementId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasDisplayName = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString DisplayName;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasDescription = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Description;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasDisplayPrice = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString DisplayPrice;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasProductType = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString ProductType;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasPeriod = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString Period;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasPeriodCount = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") int64 PeriodCount = 0;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasBillingPlan = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString BillingPlan;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasEligibilityJws = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString EligibilityJws;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasIntroductoryTerms = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FNuxieIntroductoryTerms IntroductoryTerms;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasBasePlanId = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString BasePlanId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasPurchaseOptionId = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString PurchaseOptionId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasOfferId = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") FString OfferId;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") bool bHasPricingPhases = false;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie") TArray<FNuxiePricingPhase> PricingPhases;
};

template<typename T> class TNuxieResult
{
public:
  static TNuxieResult Success(T InValue) { TNuxieResult R; R.Value = MoveTemp(InValue); return R; }
  static TNuxieResult Failure(FNuxieError InError) { TNuxieResult R; R.Error = MoveTemp(InError); return R; }
  bool IsSuccess() const { return Value.IsSet(); }
  const T& GetValue() const { check(IsSuccess()); return Value.GetValue(); }
  const FNuxieError& GetError() const { check(!IsSuccess()); return Error; }
private:
  TOptional<T> Value;
  FNuxieError Error;
};
struct FNuxieResult
{
  FNuxieError Error;
  bool IsSuccess() const { return Error.Code == ENuxieErrorCode::None; }
  const FNuxieError& GetError() const { return Error; }
};
using FNuxieCompletion = TDelegate<void(const FNuxieResult&)>;
using FNuxieIdentityCompletion = TDelegate<void(const TNuxieResult<FNuxieIdentity>&)>;
using FNuxieFeatureCompletion = TDelegate<void(const TNuxieResult<FNuxieFeatureAccess>&)>;
using FNuxieConsumeCompletion = TDelegate<void(const TNuxieResult<FNuxieUsageReceipt>&)>;

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "NuxieSubsystem.h"
#include "NuxieAsyncActions.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNuxieAsyncSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieAsyncFailure, const FNuxieError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieIdentitySuccess, const FNuxieIdentity&, Identity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieFeatureSuccess, const FNuxieFeatureAccess&, Access);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieConsumeSuccess, const FNuxieUsageReceipt&, Receipt);

UCLASS()
class NUXIE_API UNuxieConfigureAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieAsyncSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Configure (Nuxie)"))
  static UNuxieConfigureAsyncAction* Configure(UObject* WorldContextObject, const FNuxieOptions& Options);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
  UPROPERTY() FNuxieOptions Options;
};

UCLASS()
class NUXIE_API UNuxieShutdownAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieAsyncSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Shutdown (Nuxie)"))
  static UNuxieShutdownAsyncAction* Shutdown(UObject* WorldContextObject);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
};

UCLASS()
class NUXIE_API UNuxieIdentifyAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieAsyncSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Identify (Nuxie)"))
  static UNuxieIdentifyAsyncAction* Identify(UObject* WorldContextObject, const FString& CustomerId, const FNuxieIdentityOptions& Options);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
  UPROPERTY() FString CustomerId;
  UPROPERTY() FNuxieIdentityOptions Options;
};

UCLASS()
class NUXIE_API UNuxieResetAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieAsyncSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Reset (Nuxie)"))
  static UNuxieResetAsyncAction* Reset(UObject* WorldContextObject);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
};

UCLASS()
class NUXIE_API UNuxieGetIdentityAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieIdentitySuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="GetIdentity (Nuxie)"))
  static UNuxieGetIdentityAsyncAction* GetIdentity(UObject* WorldContextObject);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
};

UCLASS()
class NUXIE_API UNuxieCheckFeatureAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieFeatureSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="CheckFeature (Nuxie)"))
  static UNuxieCheckFeatureAsyncAction* CheckFeature(UObject* WorldContextObject, const FString& FeatureId, const FNuxieFeatureQuery& Query);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
  UPROPERTY() FString FeatureId;
  UPROPERTY() FNuxieFeatureQuery Query;
};

UCLASS()
class NUXIE_API UNuxieConsumeFeatureAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieConsumeSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="ConsumeFeature (Nuxie)"))
  static UNuxieConsumeFeatureAsyncAction* ConsumeFeature(UObject* WorldContextObject, const FString& FeatureId, const FNuxieFeatureCommand& Command);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
  UPROPERTY() FString FeatureId;
  UPROPERTY() FNuxieFeatureCommand Command;
};

UCLASS()
class NUXIE_API UNuxieTriggerAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieAsyncSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Trigger (Nuxie)"))
  static UNuxieTriggerAsyncAction* Trigger(UObject* WorldContextObject, const FString& EventName, const FNuxieProperties& Properties);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
  UPROPERTY() FString EventName;
  UPROPERTY() FNuxieProperties Properties;
};

UCLASS()
class NUXIE_API UNuxieDismissAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieAsyncSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Dismiss (Nuxie)"))
  static UNuxieDismissAsyncAction* Dismiss(UObject* WorldContextObject);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
};

UCLASS()
class NUXIE_API UNuxieSetLocaleAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieAsyncSuccess Success;
  UPROPERTY(BlueprintAssignable) FNuxieAsyncFailure Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="SetLocale (Nuxie)"))
  static UNuxieSetLocaleAsyncAction* SetLocale(UObject* WorldContextObject, const FString& Locale);
  void Activate() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  bool bActivated = false;
  UPROPERTY() FString Locale;
};

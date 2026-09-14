#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NuxieTypes.h"
#include "NuxieSubsystem.generated.h"
class FNuxieSession;
class UNuxiePurchaseRequest;
class UNuxieRestoreRequest;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieStatusEvent, const FNuxieStatus&, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieIdentityEvent, const FNuxieIdentity&, Identity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieFeaturesEvent, const FNuxieFeatureSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieActivityEvent, const FNuxieActivity&, Activity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieAppActionEvent, const FNuxieAppAction&, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieErrorEvent, const FNuxieError&, Error);

/** One mobile client per game instance; no Actor or GameInstance subclass required. */
UCLASS()
class NUXIE_API UNuxieSubsystem : public UGameInstanceSubsystem
{
  GENERATED_BODY()
public:
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;
  void Configure(const FNuxieOptions& Options, FNuxieCompletion Completion);
  void Shutdown(FNuxieCompletion Completion);
  void Identify(const FString& CustomerId, const FNuxieIdentityOptions& Options, FNuxieCompletion Completion);
  void Reset(FNuxieCompletion Completion);
  void GetIdentity(FNuxieIdentityCompletion Completion);
  void CheckFeature(const FString& FeatureId, const FNuxieFeatureQuery& Query, FNuxieFeatureCompletion Completion);
  void ConsumeFeature(const FString& FeatureId, const FNuxieFeatureCommand& Command, FNuxieConsumeCompletion Completion);
  void Trigger(const FString& EventName, const FNuxieProperties& Properties, FNuxieCompletion Completion);
  void Dismiss(FNuxieCompletion Completion);
  void SetLocale(const FString& Locale, FNuxieCompletion Completion);
  UFUNCTION(BlueprintPure, Category="Nuxie") FNuxieStatus GetStatus() const;
  UFUNCTION(BlueprintPure, Category="Nuxie|Features") FNuxieFeatureSnapshot GetFeatureSnapshot() const;
  UFUNCTION(BlueprintPure, Category="Nuxie|Features") FNuxieFeatureState GetFeatureState(const FString& FeatureId) const;
  UPROPERTY(BlueprintAssignable, Category="Nuxie") FNuxieStatusEvent OnStatusChanged;
  UPROPERTY(BlueprintAssignable, Category="Nuxie") FNuxieIdentityEvent OnIdentityChanged;
  UPROPERTY(BlueprintAssignable, Category="Nuxie") FNuxieFeaturesEvent OnFeaturesChanged;
  UPROPERTY(BlueprintAssignable, Category="Nuxie") FNuxieActivityEvent OnActivity;
  UPROPERTY(BlueprintAssignable, Category="Nuxie") FNuxieAppActionEvent OnAppAction;
  UPROPERTY(BlueprintAssignable, Category="Nuxie") FNuxieErrorEvent OnError;
private:
  friend class FNuxieSession;
  friend class UNuxiePurchaseRequest;
  friend class UNuxieRestoreRequest;
  TSharedPtr<FNuxieSession> Session;
  UPROPERTY() TObjectPtr<UObject> PurchaseController;
  UPROPERTY() TArray<TObjectPtr<UNuxiePurchaseRequest>> Purchases;
  UPROPERTY() TArray<TObjectPtr<UNuxieRestoreRequest>> Restores;
  void InvalidateCheckout();
};

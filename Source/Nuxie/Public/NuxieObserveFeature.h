#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "NuxieSubsystem.h"
#include "NuxieObserveFeature.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieFeatureStateEvent, const FNuxieFeatureState&, State);
/** Retain the observer to cancel it. Subscribes before delivering the current snapshot. */
UCLASS(meta=(ExposedAsyncProxy="Observer"))
class NUXIE_API UNuxieObserveFeature : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieFeatureStateEvent StateChanged;
  UPROPERTY(BlueprintReadOnly, Category="Nuxie|Features") FNuxieFeatureState State;
  UPROPERTY(BlueprintAssignable) FNuxieErrorEvent Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie|Features", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Observe Nuxie Feature"))
  static UNuxieObserveFeature* ObserveFeature(UObject* WorldContextObject, const FString& FeatureId);
  UFUNCTION(BlueprintCallable, Category="Nuxie|Features") void Cancel();
  void Activate() override;
  void BeginDestroy() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  UPROPERTY() TWeakObjectPtr<UNuxieSubsystem> Client;
  FString FeatureId;
  bool bActive = false;
  bool bFinished = false;
  UFUNCTION() void Receive(const FNuxieFeatureSnapshot& Snapshot);
};

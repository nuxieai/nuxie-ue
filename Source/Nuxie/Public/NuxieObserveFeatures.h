#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "NuxieSubsystem.h"
#include "NuxieObserveFeatures.generated.h"
/** Retain the observer to cancel it. Subscribes before delivering the current snapshot. */
UCLASS(meta=(ExposedAsyncProxy="Observer"))
class NUXIE_API UNuxieObserveFeatures : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintAssignable) FNuxieFeaturesEvent Changed;
  UPROPERTY(BlueprintAssignable) FNuxieErrorEvent Failure;
  UFUNCTION(BlueprintCallable, Category="Nuxie|Features", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
  static UNuxieObserveFeatures* ObserveFeatures(UObject* WorldContextObject);
  UFUNCTION(BlueprintCallable, Category="Nuxie|Features") void Cancel();
  void Activate() override;
  void BeginDestroy() override;
private:
  UPROPERTY() TObjectPtr<UObject> Context;
  UPROPERTY() TWeakObjectPtr<UNuxieSubsystem> Client;
  bool bActive = false;
  bool bFinished = false;
  UFUNCTION() void Receive(const FNuxieFeatureSnapshot& Snapshot);
};

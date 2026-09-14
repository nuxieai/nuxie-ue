#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NuxieTypes.h"
#include "NuxiePurchaseController.generated.h"
class FNuxieSession;
UCLASS(BlueprintType)
class NUXIE_API UNuxiePurchaseRequest : public UObject {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintReadOnly, Category="Nuxie|Billing") FNuxieStoreProduct Product;
  UFUNCTION(BlueprintCallable, Category="Nuxie|Billing") bool TryComplete(ENuxiePurchaseOutcome Outcome, const FString& Message);
  UFUNCTION(BlueprintPure, Category="Nuxie|Billing") bool IsPending() const;
  bool IsPendingAt(double Now) const;
private:
  friend class FNuxieSession;
  friend class UNuxieSubsystem;
  TWeakPtr<FNuxieSession> Session;
  FString RequestId;
  double DeadlineMs = 0;
  bool bPending = true;
};
UCLASS(BlueprintType)
class NUXIE_API UNuxieRestoreRequest : public UObject {
  GENERATED_BODY()
public:
  UFUNCTION(BlueprintCallable, Category="Nuxie|Billing") bool TryComplete(ENuxieRestoreOutcome Outcome, const FString& Message);
  UFUNCTION(BlueprintPure, Category="Nuxie|Billing") bool IsPending() const;
  bool IsPendingAt(double Now) const;
private:
  friend class FNuxieSession;
  friend class UNuxieSubsystem;
  TWeakPtr<FNuxieSession> Session;
  FString RequestId;
  double DeadlineMs = 0;
  bool bPending = true;
};
UINTERFACE(BlueprintType)
class NUXIE_API UNuxiePurchaseController : public UInterface { GENERATED_BODY() };
/** Implement asynchronous checkout using your billing provider; complete the retained request once. */
class NUXIE_API INuxiePurchaseController {
  GENERATED_BODY()
public:
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Nuxie|Billing") void BeginPurchase(UNuxiePurchaseRequest* Request);
  virtual void BeginPurchase_Implementation(UNuxiePurchaseRequest* Request);
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Nuxie|Billing") void BeginRestore(UNuxieRestoreRequest* Request);
  virtual void BeginRestore_Implementation(UNuxieRestoreRequest* Request);
};

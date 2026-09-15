#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GameFramework/SaveGame.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "NuxieSubsystem.h"
#include "NuxiePurchaseController.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NuxieLab.generated.h"
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;
/** The Lab owns its pause policy; the SDK owns native presentation. */
UCLASS()
class UNuxieLabPresentation : public UGameInstanceSubsystem {
  GENERATED_BODY()
public:
  void Initialize(FSubsystemCollectionBase& Collection) override;
  void Deinitialize() override;
  bool IsPresenting() const { return !ActiveExperiences.IsEmpty(); }
private:
  UPROPERTY() TObjectPtr<UNuxieSubsystem> Client;
  TSet<FString> ActiveExperiences;
  TArray<FString> SeenActivities;
  TWeakObjectPtr<UWorld> PausedWorld;
  bool bOwnsPause = false;
  FDelegateHandle MapLoaded;
  void ApplyPause(UWorld* World);
  void RestorePause();
  UFUNCTION() void Activity(const FNuxieActivity& Value);
  UFUNCTION() void Status(const FNuxieStatus& Value);
  UFUNCTION() void Identity(const FNuxieIdentity& Value);
};
/** Game-instance lifetime: retained checkout survives widget destruction and map travel.
 * This manual harness demonstrates cancellation/failure, never fabricates store success. */
UCLASS()
class UNuxieLabBilling : public UGameInstanceSubsystem, public INuxiePurchaseController {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintReadOnly) TObjectPtr<UNuxiePurchaseRequest> Purchase;
  UPROPERTY(BlueprintReadOnly) TObjectPtr<UNuxieRestoreRequest> Restore;
  void BeginPurchase_Implementation(UNuxiePurchaseRequest* Request) override;
  void BeginRestore_Implementation(UNuxieRestoreRequest* Request) override;
  void Deinitialize() override;
  bool CancelPurchase();
  bool FailRestore();
};
UCLASS()
class UNuxieLabSave : public USaveGame {
  GENERATED_BODY()
public:
  UPROPERTY() FString Customer;
  UPROPERTY() FString Feature;
  UPROPERTY() FNuxieFeatureCommand Command;
  UPROPERTY() TArray<FString> Applied;
  UPROPERTY() TArray<FString> Resolved;
};
UCLASS()
class UNuxieLabButton : public UButton {
  GENERATED_BODY()
public:
  TFunction<void()> Action;
  UFUNCTION() void Run() { if (Action) Action(); }
};
UCLASS()
class UNuxieLabWidget : public UUserWidget {
  GENERATED_BODY()
protected:
  TSharedRef<SWidget> RebuildWidget() override;
  void NativeDestruct() override;
  void NativeConstruct() override;
private:
  UPROPERTY() TObjectPtr<UNuxieSubsystem> Client;
  UPROPERTY() TObjectPtr<UNuxieLabSave> Save;
  UPROPERTY() TObjectPtr<UEditableTextBox> PublicKey;
  UPROPERTY() TObjectPtr<UEditableTextBox> Customer;
  UPROPERTY() TObjectPtr<UEditableTextBox> Feature;
  UPROPERTY() TObjectPtr<UEditableTextBox> Entity;
  UPROPERTY() TObjectPtr<UEditableTextBox> Event;
  UPROPERTY() TObjectPtr<UEditableTextBox> ComparisonEntity;
  bool bOperationPending = false;
  void ValidationFinished(bool bPassed, const FString& Message);
  UPROPERTY() TObjectPtr<UTextBlock> Output;
  UPROPERTY() TObjectPtr<UVerticalBox> Panel;
  void Log(const FString& Text);
  void Button(const FString& Label, TFunction<void()> Action);
  UEditableTextBox* Input(const FString& Hint, const FString& Value);
  void Consume();
  void Validate();
  void RunLifecycle(int32 Step);
  void LifecycleFinished(bool bPassed, const FString& Message);
  FNuxieCompletion Completion(const FString& Label);
  UFUNCTION() void FeaturesChanged(const FNuxieFeatureSnapshot& Snapshot);
  UFUNCTION() void Activity(const FNuxieActivity& Value);
  UFUNCTION() void AppAction(const FNuxieAppAction& Value);
};
UCLASS()
class ANuxieLabController : public APlayerController {
  GENERATED_BODY()
protected:
  void BeginPlay() override;
};
UCLASS()
class ANuxieLabGameMode : public AGameModeBase {
  GENERATED_BODY()
public:
  ANuxieLabGameMode();
};

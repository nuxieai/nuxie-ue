#include "NuxieObserveFeatures.h"
#include "NuxieSession.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
UNuxieObserveFeatures* UNuxieObserveFeatures::ObserveFeatures(UObject* WorldContextObject) { auto* Observer = NewObject<UNuxieObserveFeatures>(); Observer->Context = WorldContextObject; return Observer; }
void UNuxieObserveFeatures::Activate() {
  if (bActive || bFinished) return;
  auto* Instance = Cast<UGameInstance>(Context);
  if (!Instance) { auto* World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull); if (World) Instance = World->GetGameInstance(); }
  if (!Instance) { Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A game instance is required."))); Cancel(); return; }
  RegisterWithGameInstance(Instance);
  Client = Instance->GetSubsystem<UNuxieSubsystem>();
  bActive = true;
  Client->OnFeaturesChanged.AddDynamic(this, &UNuxieObserveFeatures::Receive);
  Receive(Client->GetFeatureSnapshot());
}
void UNuxieObserveFeatures::Receive(const FNuxieFeatureSnapshot& Snapshot) { if (bActive && !bFinished) Changed.Broadcast(Snapshot); }
void UNuxieObserveFeatures::Cancel() {
  if (Client.IsValid()) Client->OnFeaturesChanged.RemoveDynamic(this, &UNuxieObserveFeatures::Receive);
  Client.Reset(); Context = nullptr; bActive = false; bFinished = true; SetReadyToDestroy();
}
void UNuxieObserveFeatures::BeginDestroy() { Cancel(); Super::BeginDestroy(); }

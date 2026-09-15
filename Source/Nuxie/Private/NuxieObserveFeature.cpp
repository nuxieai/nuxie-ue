#include "NuxieObserveFeature.h"
#include "NuxieSession.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
UNuxieObserveFeature* UNuxieObserveFeature::ObserveFeature(UObject* WorldContextObject, const FString& FeatureId) { auto* Observer = NewObject<UNuxieObserveFeature>(); Observer->Context = WorldContextObject; Observer->FeatureId = FeatureId; return Observer; }
void UNuxieObserveFeature::Activate() {
  if (bActive || bFinished) return;
  if (FeatureId.TrimStartAndEnd().IsEmpty()) { Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A feature ID is required."))); Cancel(); return; }
  auto* Instance = Cast<UGameInstance>(Context);
  if (!Instance) { auto* World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull); if (World) Instance = World->GetGameInstance(); }
  if (!Instance) { Failure.Broadcast(NuxieWire::Error(ENuxieErrorCode::InvalidArgument, TEXT("A game instance is required."))); Cancel(); return; }
  RegisterWithGameInstance(Instance);
  Client = Instance->GetSubsystem<UNuxieSubsystem>();
  bActive = true;
  Client->OnDeinitializing.AddUObject(this, &UNuxieObserveFeature::Cancel);
  Client->OnFeaturesChanged.AddDynamic(this, &UNuxieObserveFeature::Receive);
  Receive(Client->GetFeatureSnapshot());
}
void UNuxieObserveFeature::Receive(const FNuxieFeatureSnapshot& Snapshot) { if (bActive && !bFinished) { State = Snapshot.Select(FeatureId); StateChanged.Broadcast(State); } }
void UNuxieObserveFeature::Cancel() {
  if (Client.IsValid()) Client->OnFeaturesChanged.RemoveDynamic(this, &UNuxieObserveFeature::Receive);
  if (Client.IsValid()) Client->OnDeinitializing.RemoveAll(this);
  Client.Reset(); Context = nullptr; bActive = false; bFinished = true; SetReadyToDestroy();
}
void UNuxieObserveFeature::BeginDestroy() { Cancel(); Super::BeginDestroy(); }

#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NuxieTypes.h"
#include "NuxieSettings.generated.h"
/** Public keys are safe to ship. Never enter backend secret keys here. Configuration remains explicit. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Nuxie"))
class NUXIE_API UNuxieSettings : public UDeveloperSettings {
  GENERATED_BODY()
public:
  UPROPERTY(Config, EditAnywhere, Category="Nuxie") FString IOSPublicKey;
  UPROPERTY(Config, EditAnywhere, Category="Nuxie") FString AndroidPublicKey;
  UPROPERTY(Config, EditAnywhere, Category="Nuxie") ENuxieEnvironment Environment = ENuxieEnvironment::Production;
  UPROPERTY(Config, EditAnywhere, Category="Nuxie") ENuxieLogLevel LogLevel = ENuxieLogLevel::Warning;
  UFUNCTION(BlueprintPure, Category="Nuxie") static FNuxieOptions GetProjectOptions();
};

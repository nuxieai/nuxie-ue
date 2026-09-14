#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NuxieTypes.h"
#include "NuxieBlueprintLibrary.generated.h"
UCLASS()
class NUXIE_API UNuxieBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  UFUNCTION(BlueprintCallable, Category="Nuxie|Values") static bool ParseProperties(const FString& Json, FNuxieProperties& Properties, FString& Error);
  UFUNCTION(BlueprintCallable, Category="Nuxie|Values") static bool ParseValue(const FString& Json, FNuxieJsonValue& Value, FString& Error);
  UFUNCTION(BlueprintPure, Category="Nuxie|Values") static FNuxieJsonValue StringValue(const FString& Value);
  UFUNCTION(BlueprintPure, Category="Nuxie|Values") static FNuxieJsonValue BoolValue(bool Value);
  UFUNCTION(BlueprintCallable, Category="Nuxie|Values") static bool NumberValue(double Number, FNuxieJsonValue& Value, FString& Error);
  UFUNCTION(BlueprintPure, Category="Nuxie|Values") static FNuxieJsonValue NullValue();
  UFUNCTION(BlueprintPure, Category="Nuxie|Values") static FNuxieJsonValue ObjectValue(const FNuxieProperties& Properties);
  UFUNCTION(BlueprintCallable, Category="Nuxie|Values") static bool ArrayValue(const TArray<FNuxieJsonValue>& Values, FNuxieJsonValue& Value, FString& Error);
  UFUNCTION(BlueprintCallable, Category="Nuxie|Values") static bool WithProperty(const FNuxieProperties& Properties, const FString& Name, const FNuxieJsonValue& Value, FNuxieProperties& Result, FString& Error);
  UFUNCTION(BlueprintPure, Category="Nuxie|Values") static FString PropertiesToJson(const FNuxieProperties& Properties);
  UFUNCTION(BlueprintPure, Category="Nuxie|Values") static FString ValueToJson(const FNuxieJsonValue& Value);
};

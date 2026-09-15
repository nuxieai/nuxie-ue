#include "NuxieBlueprintLibrary.h"
#include "NuxieWire.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace {
TSharedPtr<FJsonValue> Parse(const FNuxieJsonValue& Value) { return NuxieWire::Value(Value.ToJson()); }
FNuxieJsonValue Encode(const TSharedRef<FJsonValue>& Value) { FString Json = NuxieWire::JsonValue(Value), Error; FNuxieJsonValue Result; FNuxieJsonValue::TryParse(Json, Result, Error); return Result; }
}
bool UNuxieBlueprintLibrary::ParseProperties(const FString& Json, FNuxieProperties& Properties, FString& Error) { return FNuxieProperties::TryParse(Json, Properties, Error); }
bool UNuxieBlueprintLibrary::ParseValue(const FString& Json, FNuxieJsonValue& Value, FString& Error) { return FNuxieJsonValue::TryParse(Json, Value, Error); }
FNuxieJsonValue UNuxieBlueprintLibrary::StringValue(const FString& Value) { return Encode(MakeShared<FJsonValueString>(Value)); }
FNuxieJsonValue UNuxieBlueprintLibrary::BoolValue(bool Value) { return Encode(MakeShared<FJsonValueBoolean>(Value)); }
bool UNuxieBlueprintLibrary::NumberValue(double Number, FNuxieJsonValue& Value, FString& Error) {
  if (!FMath::IsFinite(Number) || (FMath::FloorToDouble(Number) == Number && FMath::Abs(Number) > 9007199254740991.0)) { Error = TEXT("Numbers must be finite; integers must be within the exact JSON range."); return false; }
  Value = Encode(MakeShared<FJsonValueNumber>(Number)); Error.Reset(); return true;
}
FNuxieJsonValue UNuxieBlueprintLibrary::NullValue() { return FNuxieJsonValue(); }
FNuxieJsonValue UNuxieBlueprintLibrary::ObjectValue(const FNuxieProperties& Properties) { FNuxieJsonValue Result; FString Error; FNuxieJsonValue::TryParse(Properties.ToJson(), Result, Error); return Result; }
bool UNuxieBlueprintLibrary::ArrayValue(const TArray<FNuxieJsonValue>& Values, FNuxieJsonValue& Value, FString& Error) {
  TArray<TSharedPtr<FJsonValue>> Array; for (const auto& Item : Values) Array.Add(Parse(Item));
  FString Json; FJsonSerializer::Serialize(MakeShared<FJsonValueArray>(Array), TEXT(""), TJsonWriterFactory<>::Create(&Json));
  return FNuxieJsonValue::TryParse(Json, Value, Error);
}
bool UNuxieBlueprintLibrary::WithProperty(const FNuxieProperties& Properties, const FString& Name, const FNuxieJsonValue& Value, FNuxieProperties& Result, FString& Error) {
  auto Object = NuxieWire::Object(Properties.ToJson()); auto Field = Parse(Value);
  if (!Object || !Field) { Error = TEXT("Invalid stored JSON value."); return false; }
  Object->SetField(Name, Field);
  return FNuxieProperties::TryParse(NuxieWire::Json(Object), Result, Error);
}
FString UNuxieBlueprintLibrary::PropertiesToJson(const FNuxieProperties& Properties) { return Properties.ToJson(); }
FString UNuxieBlueprintLibrary::ValueToJson(const FNuxieJsonValue& Value) { return Value.ToJson(); }

bool UNuxieBlueprintLibrary::IntegerValue(int64 Integer, FNuxieJsonValue& Value, FString& Error) {
  if (Integer < -9007199254740991LL || Integer > 9007199254740991LL) { Error = TEXT("Integer is outside the exact JSON range."); return false; }
  return FNuxieJsonValue::TryParse(FString::Printf(TEXT("%lld"), Integer), Value, Error);
}

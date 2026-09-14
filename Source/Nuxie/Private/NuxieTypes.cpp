#include "NuxieTypes.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace {
bool Portable(const TSharedPtr<FJsonValue>& Value, int32 Depth) {
  if (!Value || Depth > 32) return false;
  switch (Value->Type) {
    case EJson::Null: case EJson::String: case EJson::Boolean: return true;
    case EJson::Number: {
      const double Number = Value->AsNumber();
      return FMath::IsFinite(Number) && (FMath::FloorToDouble(Number) != Number || FMath::Abs(Number) <= 9007199254740991.0);
    }
    case EJson::Array:
      for (const auto& Child : Value->AsArray()) if (!Portable(Child, Depth + 1)) return false;
      return true;
    case EJson::Object:
      for (const auto& Pair : Value->AsObject()->Values) if (!Portable(Pair.Value, Depth + 1)) return false;
      return true;
    default: return false;
  }
}
bool Parse(const FString& Input, bool bObject, FString& Output) {
  TSharedPtr<FJsonValue> Value;
  if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Input), Value) || !Portable(Value, 0)) return false;
  if (bObject && Value->Type != EJson::Object) return false;
  Output.Reset();
  return FJsonSerializer::Serialize(Value.ToSharedRef(), TEXT(""), TJsonWriterFactory<>::Create(&Output));
}
}
bool FNuxieProperties::TryParse(const FString& Input, FNuxieProperties& Out, FString& Error) {
  FString Json;
  if (!Parse(Input, true, Json)) { Error = TEXT("Expected a JSON object with finite, portable numbers and depth at most 32."); return false; }
  Error.Reset();
  Out.Json = MoveTemp(Json);
  return true;
}
bool FNuxieJsonValue::TryParse(const FString& Input, FNuxieJsonValue& Out, FString& Error) {
  FString Json;
  if (!Parse(Input, false, Json)) { Error = TEXT("Expected JSON with finite, portable numbers and depth at most 32."); return false; }
  Error.Reset();
  Out.Json = MoveTemp(Json);
  return true;
}

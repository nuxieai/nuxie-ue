#include "NuxieTypes.h"
#include "NuxieBlueprintLibrary.h"
#include "NuxieWire.h"
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
  auto Value = NuxieWire::Value(Input);
  if (!Portable(Value, 0)) return false;
  if (bObject && Value->Type != EJson::Object) return false;
  Output = NuxieWire::JsonValue(Value);
  return !Output.IsEmpty();
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

bool FNuxieProperties::WithString(const FString& Name, const FString& Value, FString& Error) { return UNuxieBlueprintLibrary::WithProperty(*this, Name, UNuxieBlueprintLibrary::StringValue(Value), *this, Error); }
bool FNuxieProperties::WithBool(const FString& Name, bool Value, FString& Error) { return UNuxieBlueprintLibrary::WithProperty(*this, Name, UNuxieBlueprintLibrary::BoolValue(Value), *this, Error); }
bool FNuxieProperties::WithInteger(const FString& Name, int64 Integer, FString& Error) { FNuxieJsonValue Value; return UNuxieBlueprintLibrary::IntegerValue(Integer, Value, Error) && UNuxieBlueprintLibrary::WithProperty(*this, Name, Value, *this, Error); }
bool FNuxieProperties::WithNumber(const FString& Name, double Number, FString& Error) { FNuxieJsonValue Value; return UNuxieBlueprintLibrary::NumberValue(Number, Value, Error) && UNuxieBlueprintLibrary::WithProperty(*this, Name, Value, *this, Error); }
bool FNuxieProperties::WithNull(const FString& Name, FString& Error) { return UNuxieBlueprintLibrary::WithProperty(*this, Name, UNuxieBlueprintLibrary::NullValue(), *this, Error); }
bool FNuxieProperties::WithObject(const FString& Name, const FNuxieProperties& Value, FString& Error) { return UNuxieBlueprintLibrary::WithProperty(*this, Name, UNuxieBlueprintLibrary::ObjectValue(Value), *this, Error); }
bool FNuxieProperties::WithArray(const FString& Name, const TArray<FNuxieJsonValue>& Values, FString& Error) { FNuxieJsonValue Value; return UNuxieBlueprintLibrary::ArrayValue(Values, Value, Error) && UNuxieBlueprintLibrary::WithProperty(*this, Name, Value, *this, Error); }
